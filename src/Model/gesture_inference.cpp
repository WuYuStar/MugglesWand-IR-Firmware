#include "gesture_inference.h"
#include "Service/DualPrint.h"
#include "Service/EventBus.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "gesture_buffer.h"
#include "ModelStore.h"
#include <esp_heap_caps.h>
#include <math.h>
#include <new>
#include "Service/AP.h"
#include "Service/AP_Web.h"
#include "Service/GestureBindings.h"
#include "HAL/HAL.h"

TaskHandle_t inference_task_handle = NULL;

namespace
{
    tflite::MicroErrorReporter error_reporter;
    tflite::AllOpsResolver &sharedResolver(){static tflite::AllOpsResolver value;return value;}
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;

    constexpr int kTensorArenaSize = 50 * 1024;
#ifdef WAND_PSRAM_ARENA
    uint8_t *tensor_arena = nullptr;
#else
    alignas(16) uint8_t tensor_arena[kTensorArenaSize];
#endif
}

void handleGesture(int gesture_id, float confidence,uint32_t generation)
{
    if (gesture_id<0 || gesture_id>=gesture_model_count()) return;
    const auto config=gesture_bindings_state();
    const bool accepted=confidence>=0.6f;
    ap_web_gesture_result(gesture_id,confidence,accepted,config.actions[gesture_id],config.effects[gesture_id]);
    if (accepted) EventBus::publish(EVENT_GESTURE_TRIGGER,config.actions[gesture_id],config.effects[gesture_id],generation);
}

void inference_task(void *pvParameters)
{
    // 初始化缓冲区

    // 注册到缓冲区系统
    g_gesture_buffer.inference_task = xTaskGetCurrentTaskHandle();

    while (1)
    {
        // 等待缓冲区准备好
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        float buffer[kWindowSize*kNumFeatures];
        uint32_t generation;

        // 获取缓冲区
        while (acquireBuffer(buffer,generation))
        {
            if(ModelStore::busy()||HAL::ICM42670P_getDataMux()!=2)continue;
            const auto mean=ModelStore::mean(),std=ModelStore::stddev();
            const int kNumClasses=gesture_model_count();
            int64_t start_time = esp_timer_get_time();

            // 准备输入数据（归一化 + 量化）
            for (int i = 0; i < kWindowSize * kNumFeatures; i++)
            {
                int feature_idx = i % kNumFeatures;
                float normalized = (buffer[i] - mean[feature_idx]) / std[feature_idx];

                // 量化
                float raw=roundf(normalized/input->params.scale+input->params.zero_point);
                int8_t quantized=static_cast<int8_t>(raw<-128?-128:raw>127?127:raw);
                input->data.int8[i] = quantized;
            }

            // 运行推理
            if (interpreter->Invoke() != kTfLiteOk)
            {
                DualSerial.println("Invoke failed!");

                continue;
            }

            // 处理输出
            int8_t max_val = -128;
            int predicted_class = 0;
            float probabilities[kMaxGestureClasses]; // 用于存储反量化后的概率值

            for (int i = 0; i < kNumClasses; i++)
            {
                int8_t val = output->data.int8[i];

                // 反量化公式: real_value = (quantized_value - zero_point) * scale
                probabilities[i] = (val - output->params.zero_point) * output->params.scale;

                if (val > max_val)
                {
                    max_val = val;
                    predicted_class = i;
                }
            }

            int64_t end_time = esp_timer_get_time();
            float inference_time = (end_time - start_time) / 1000.0f; // ms
            float max_probability = probabilities[predicted_class];

            // 输出推理结果, 包括类别、概率和推理时间
            DualSerial.printf("[CNN] C:%d P:%.2f T:%.0fms\n", predicted_class, max_probability, inference_time);

            if(gestureBufferCurrent(generation)&&HAL::ICM42670P_getDataMux()==2&&!ModelStore::busy())
                handleGesture(predicted_class,max_probability,generation);
        }
    }
}

void inference_start()
{
    if (inference_task_handle != NULL)
    {
        DualSerial.println("[CNN] Task already running");
        return;
    }

    if (interpreter == nullptr || input == nullptr || output == nullptr || !g_gesture_buffer.mutex)
    {
        DualSerial.println("[CNN] Not initialized");
        return;
    }

    xTaskCreatePinnedToCore(
        inference_task,
        "Inference",
        8192,
        NULL,
        1,
        &inference_task_handle,
        1);
}

bool inference_validate_model(const uint8_t *data,size_t size,int classes,char *error,size_t capacity)
{
    flatbuffers::Verifier verifier(data,size);
    if(!tflite::VerifyModelBuffer(verifier)){strlcpy(error,"模型文件结构无效",capacity);return false;}
    const auto candidate=tflite::GetModel(data);
    if(candidate->version()!=TFLITE_SCHEMA_VERSION){strlcpy(error,"模型格式版本不兼容",capacity);return false;}
    auto graphs=candidate->subgraphs();
    if(!graphs||!graphs->size()||!candidate->buffers()||!candidate->operator_codes()){
        strlcpy(error,"模型缺少必要的数据表",capacity);return false;
    }
    auto graph=graphs->Get(0);
    if(!graph->tensors()||!graph->inputs()||!graph->outputs()||graph->inputs()->size()!=1||graph->outputs()->size()!=1||
       graph->inputs()->Get(0)<0||graph->outputs()->Get(0)<0||
       static_cast<unsigned>(graph->inputs()->Get(0))>=graph->tensors()->size()||static_cast<unsigned>(graph->outputs()->Get(0))>=graph->tensors()->size()){
        strlcpy(error,"模型输入输出索引无效",capacity);return false;
    }
    auto *arena=ModelStore::allocate(kTensorArenaSize);
    if(!arena){strlcpy(error,"模型验证内存不足，原模型未改动",capacity);return false;}
    bool ok=false;
    {
        tflite::MicroInterpreter test(candidate,sharedResolver(),arena,kTensorArenaSize,&error_reporter);
        if(test.AllocateTensors()==kTfLiteOk){
            auto in=test.input(0),out=test.output(0);
            ok=in&&out&&in->type==kTfLiteInt8&&out->type==kTfLiteInt8&&in->bytes==200&&out->bytes==classes&&in->params.zero_point>=-128&&in->params.zero_point<=127&&out->params.zero_point>=-128&&out->params.zero_point<=127&&in->dims->size==3&&in->dims->data[0]==1&&in->dims->data[1]==100&&in->dims->data[2]==2&&out->dims->size==2&&out->dims->data[0]==1&&out->dims->data[1]==classes&&isfinite(in->params.scale)&&in->params.scale>0&&isfinite(out->params.scale)&&out->params.scale>0;
            if(ok){memset(in->data.int8,in->params.zero_point,in->bytes);ok=test.Invoke()==kTfLiteOk;}
        }
    }
    free(arena);
    if(!ok)strlcpy(error,"模型算子、尺寸或内存需求与魔杖不兼容",capacity);
    return ok;
}

void inference_init()
{
    if(interpreter)return;
    initGestureBuffer();
#ifdef WAND_PSRAM_ARENA
    if (!tensor_arena) {
        tensor_arena = static_cast<uint8_t *>(heap_caps_aligned_alloc(16, kTensorArenaSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!tensor_arena) {
            tensor_arena = static_cast<uint8_t *>(heap_caps_aligned_alloc(16, kTensorArenaSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        }
        if (!tensor_arena) {
            DualSerial.println("[Model] Tensor arena allocation failed");
            return;
        }
    }
#endif
    alignas(tflite::MicroInterpreter) static uint8_t storage[sizeof(tflite::MicroInterpreter)];
    for(int attempt=0;attempt<2;++attempt){
        model=tflite::GetModel(ModelStore::model_data());
        if(model->version()!=TFLITE_SCHEMA_VERSION)break;
        interpreter=new(storage)tflite::MicroInterpreter(model,sharedResolver(),tensor_arena,kTensorArenaSize,&error_reporter);
        if(interpreter->AllocateTensors()==kTfLiteOk){input=interpreter->input(0);output=interpreter->output(0);return;}
        interpreter->~MicroInterpreter();interpreter=nullptr;ModelStore::use_builtin();
    }
    input=output=nullptr;DualSerial.println("[Model] No compatible model available");
}

bool inference_ready(){return interpreter&&input&&output&&inference_task_handle&&gestureBufferReady();}
