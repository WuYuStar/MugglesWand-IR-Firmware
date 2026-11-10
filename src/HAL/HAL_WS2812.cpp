#include "HAL.h"
#include <Arduino.h>
#include "Config.h"
#include <FastLED.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <atomic>

// 引入动画基类
#include "WS2812_Animation/AnimationBase.hpp"

// ==================== 静态硬件资源 ====================
static CRGB leds[WS2812_LED_COUNT];
static TaskHandle_t ws2812_task_handle = NULL;

// ==================== 图层指针 (线程安全队列间接控制) ====================
static AnimationBase *bg_layer = nullptr;      // 背景层（持续性，如充电、普通呼吸）
static AnimationBase *fx_layer = nullptr;      // 特效层（一次性，如魔法光流 Flow）
static AnimationBase *overlay_layer = nullptr; // 顶层覆盖（最高优先级，如手势指示灯）

// ==================== 多任务同步队列 ====================
typedef enum
{
    CMD_SET_BG,
    CMD_START_FX,
    CMD_SET_OVERLAY
} layer_cmd_type_t;

typedef struct
{
    layer_cmd_type_t type;
    AnimationBase *anim_ptr;
} layer_cmd_t;

static QueueHandle_t layer_queue = NULL;
static SemaphoreHandle_t layerMutex=nullptr,stopped=nullptr;
static std::atomic<bool> stopping{false};
static void clear_layers(){
    delete bg_layer;delete fx_layer;delete overlay_layer;
    bg_layer=fx_layer=overlay_layer=nullptr;
    layer_cmd_t cmd;
    while(xQueueReceive(layer_queue,&cmd,0)==pdTRUE)delete cmd.anim_ptr;
    FastLED.clear(true);pinMode(PIN_WS2812_EN,INPUT);
}

// ==================== 核心渲染任务循环 ====================
static void ws2812_task(void *pvParameters)
{
    (void)pvParameters;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10); // 严格的 10ms 刷新周期（vTaskDelayUntil 锚定绝对时刻）

    // 用于记录当前硬件电源状态
    bool is_power_on = false;

    for (;;)
    {
        if(stopping.load()){
            clear_layers();xSemaphoreGive(stopped);vTaskDelete(nullptr);return;
        }
        // 1. 检查是否有上层 APP / Service 发来的图层切换命令
        layer_cmd_t cmd;
        while (xQueueReceive(layer_queue, &cmd, 0) == pdTRUE)
        {
            switch (cmd.type)
            {
            case CMD_SET_BG:
                if (bg_layer)
                    delete bg_layer;
                bg_layer = cmd.anim_ptr;
                if (bg_layer)
                    bg_layer->init();
                break;

            case CMD_START_FX:
                if (fx_layer)
                    delete fx_layer;
                fx_layer = cmd.anim_ptr;
                if (fx_layer)
                    fx_layer->init();
                break;

            case CMD_SET_OVERLAY:
                if (overlay_layer)
                    delete overlay_layer;
                overlay_layer = cmd.anim_ptr;
                if (overlay_layer)
                    overlay_layer->init();
                break;
            }
        }

        // 判断当前是否有任意一个图层在工作
        bool has_active_animation = (bg_layer != nullptr || fx_layer != nullptr || overlay_layer != nullptr);

        if (!has_active_animation)
        {
            // 如果所有图层都为空，且电源还开着 -> 关闭电源
            if (is_power_on)
            {
                pinMode(PIN_WS2812_EN, INPUT); // 关闭灯带电源
                is_power_on = false;
            }

            // 电源关闭期间，无需频繁执行后面的渲染和 show()，直接延时进入下一轮
            vTaskDelayUntil(&xLastWakeTime, xFrequency);
            continue;
        }
        else
        {
            // 如果有动画要播放，且当前电源是关着的 -> 开启电源
            if (!is_power_on)
            {
                pinMode(PIN_WS2812_EN, OUTPUT);
                digitalWrite(PIN_WS2812_EN, 0); // 对应 init 中的开启电源操作
                is_power_on = true;
            }
        }

        // 2. 彻底清除硬件缓冲区画布
        fill_solid(leds, WS2812_LED_COUNT, CRGB::Black);

        // 3. 【渲染第一层】 背景层
        if (bg_layer)
        {
            bg_layer->update(leds, WS2812_LED_COUNT);
        }

        // 4. 【渲染第二层】 特效层 (覆盖或混合)
        if (fx_layer)
        {
            fx_layer->update(leds, WS2812_LED_COUNT);
            // 如果是一次性动画（如Flow跑完了），引擎自动销毁释放内存，完美暴露出底色
            if (fx_layer->finished())
            {
                delete fx_layer;
                fx_layer = nullptr;
            }
        }

        // 5. 【渲染第三层】 顶层悬浮（强制覆盖，手势状态灯绝对可见）
        if (overlay_layer)
        {
            overlay_layer->update(leds, WS2812_LED_COUNT);
            if (overlay_layer->finished())
            {
                delete overlay_layer;
                overlay_layer = nullptr;
            }
        }

        // 6. 刷向物理引脚驱动硬件
        FastLED.show();

        // 7. 绝对延时锁定帧率
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// ==================== HAL 层开放接口实现 ====================

void HAL::ws2812_init(void)
{
    // 默认先关闭灯带电源
    pinMode(PIN_WS2812_EN, INPUT);
    // 初始化 FastLED 物理外设
    FastLED.addLeds<WS2812, PIN_WS2812, GRB>(leds, WS2812_LED_COUNT);
    FastLED.setBrightness(255); // 默认全亮，交由动画层自己调节亮度
    FastLED.clear();

    // 创建命令通信队列 (深度为8，防突发堆积)
    layerMutex=xSemaphoreCreateMutex();stopped=xSemaphoreCreateBinary();
    layer_queue = xQueueCreate(8, sizeof(layer_cmd_t));
    if (!layer_queue||!layerMutex||!stopped)
        return;

    // 创建渲染核心任务
    xTaskCreatePinnedToCore(
        ws2812_task,
        "WS2812_Engine_Task",
        4096,
        NULL,
        4,
        &ws2812_task_handle,
        1);
}

// 停止引擎并清理内存
void HAL::ws2812_stop(void)
{
    if(!layerMutex||xSemaphoreTake(layerMutex,pdMS_TO_TICKS(100))!=pdTRUE)return;
    stopping.store(true);
    if(ws2812_task_handle&&xSemaphoreTake(stopped,pdMS_TO_TICKS(1000))==pdTRUE)
        ws2812_task_handle=nullptr;
    // Queue/layer ownership stays with the renderer until it acknowledges stop.
    xSemaphoreGive(layerMutex);
}

static void set_layer(layer_cmd_type_t type,AnimationBase *anim){
    if(!layerMutex||xSemaphoreTake(layerMutex,pdMS_TO_TICKS(10))!=pdTRUE){delete anim;return;}
    layer_cmd_t command{type,anim};
    bool accepted=!stopping.load()&&ws2812_task_handle&&layer_queue&&xQueueSend(layer_queue,&command,0)==pdTRUE;
    xSemaphoreGive(layerMutex);
    if(!accepted)delete anim;
}
void HAL::ws2812_set_background(AnimationBase *anim){set_layer(CMD_SET_BG,anim);}
void HAL::ws2812_start_fx(AnimationBase *anim){set_layer(CMD_START_FX,anim);}
void HAL::ws2812_set_overlay(AnimationBase *anim){set_layer(CMD_SET_OVERLAY,anim);}
