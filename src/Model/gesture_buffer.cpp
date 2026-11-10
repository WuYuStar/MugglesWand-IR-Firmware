#include "gesture_buffer.h"
#include <string.h>
#include <math.h>

GestureBuffer g_gesture_buffer{};

void initGestureBuffer(){
    if(!g_gesture_buffer.mutex)g_gesture_buffer.mutex=xSemaphoreCreateMutex();
    resetGestureBuffer();
}
uint32_t resetGestureBuffer(){
    if(!g_gesture_buffer.mutex)return 0;
    xSemaphoreTake(g_gesture_buffer.mutex,portMAX_DELAY);
    g_gesture_buffer.write_idx=g_gesture_buffer.total_samples=0;
    g_gesture_buffer.data_ready=false;
    uint32_t generation=++g_gesture_buffer.generation;
    xSemaphoreGive(g_gesture_buffer.mutex);return generation;
}
void addSample(float x,float y,bool trigger){
    if(!g_gesture_buffer.mutex||!isfinite(x)||!isfinite(y))return;
    xSemaphoreTake(g_gesture_buffer.mutex,portMAX_DELAY);
    auto &b=g_gesture_buffer;
    b.history_buffer[b.write_idx][0]=x;b.history_buffer[b.write_idx][1]=y;
    b.write_idx=(b.write_idx+1)%kWindowSize;
    if(b.total_samples<kWindowSize)++b.total_samples;
    if(trigger&&b.total_samples==kWindowSize){
        for(int i=0;i<kWindowSize;++i){
            int src=(b.write_idx+i)%kWindowSize;
            memcpy(b.inference_data[i],b.history_buffer[src],sizeof(b.inference_data[i]));
        }
        b.data_ready=true;
        if(b.inference_task)xTaskNotifyGive(b.inference_task);
    }
    xSemaphoreGive(b.mutex);
}
bool acquireBuffer(float *out,uint32_t &generation){
    if(!g_gesture_buffer.mutex)return false;
    xSemaphoreTake(g_gesture_buffer.mutex,portMAX_DELAY);
    auto &b=g_gesture_buffer;bool ready=b.data_ready;
    if(ready){memcpy(out,b.inference_data,sizeof(b.inference_data));generation=b.generation;b.data_ready=false;}
    xSemaphoreGive(b.mutex);return ready;
}
bool gestureBufferCurrent(uint32_t generation){
    if(!g_gesture_buffer.mutex)return false;
    xSemaphoreTake(g_gesture_buffer.mutex,portMAX_DELAY);
    bool current=generation==g_gesture_buffer.generation;
    xSemaphoreGive(g_gesture_buffer.mutex);return current;
}

bool gestureBufferReady(){
    if(!g_gesture_buffer.mutex)return false;
    xSemaphoreTake(g_gesture_buffer.mutex,portMAX_DELAY);
    bool ready=g_gesture_buffer.total_samples==kWindowSize;
    xSemaphoreGive(g_gesture_buffer.mutex);return ready;
}
