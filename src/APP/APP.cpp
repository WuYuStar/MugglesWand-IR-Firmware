#include "APP.h"
#include "Service/HomeAssistant.h"
#include "Service/GestureBindings.h"
#include "APP_Lumos.h"
#include "APP_BLE_HID.h"
#include "APP_EspNow.h"
#include "APP_POV.h"
#include "Service/EventBus.h"
#include "Service/AP.h"
#include "Service/IrLink.h"
#include "Service/GestureEffects.h"
#include "Model/ModelStore.h"
#include "Model/gesture_buffer.h"
#include "HAL/HAL.h"
#include "HAL/WS2812_Animation/AnimFlow.hpp"
#include "HAL/WS2812_Animation/AnimBlink.hpp"

QueueHandle_t app_queue = NULL;

// Run on the APP task so switching modes cannot leave multiple apps active.
static void exit_active_modes()
{
    if (APP_ble_mouse_in_mode()) APP_ble_mouse_mode_exit();
    if (APP_ble_volume_in_mode()) APP_ble_volume_mode_exit();
    if (APP_POV_in_mode()) APP_POV_mode_exit();
}

static void app_task(void *pvParameters)
{
    SystemEvent event;

    while (1)
    {
        if (xQueueReceive(app_queue, &event, portMAX_DELAY))
        {
            if(ModelStore::busy())continue;
            switch (event.id)
            {
            case EVENT_APP_SET_MODE:
                exit_active_modes();
                EventBus::publish(event.param1.i32 == -1 ? EVENT_IMU_RESET_MUX : EVENT_IMU_SET_MUX,
                                  event.param1.i32);
                break;

            case EVENT_GESTURE_FX_PREVIEW:
                gesture_effect_play(event.param1.i32);
                break;
            case EVENT_GESTURE_TRIGGER:
                if(!gestureBufferCurrent(event.generation))break;
            case EVENT_GESTURE_DETECTED:
            {
                const int effect=event.id==EVENT_GESTURE_TRIGGER?event.param2.i32:-1;
                if(event.param1.i32 < -1 || event.param1.i32>kLastGestureAction || gesture_action_retired(event.param1.i32))break;
                if (event.param1.i32>=0) exit_active_modes();
                switch (event.param1.i32)
                {
                case 4:
                    APP_ble_mouse_mode_enter(); // 鼠标模式
                    break;
                case 5:
                    APP_Lumos_trigger(CRGB::White); // Lumos开关
                    break;
                case 6:
                    APP_ble_volume_mode_enter(); // 音量旋钮模式
                    break;
                case 7:
                    APP_POV_mode_enter(); // POV 光绘模式
                    break;
                default:
                    if (event.param1.i32 >= 12 && event.param1.i32 <= 43) {
                        ir_link_command(0x42, static_cast<uint8_t>(event.param1.i32 - 12));
                    } else if (event.param1.i32 >= kFirstHaAction) ha_send(event.param1.i32);
                    break;
                }
                if(event.param1.i32>=0){
                    int action=event.param1.i32;
                    EventBus::publish(EVENT_IMU_SET_MUX,action==4?3:action==6?4:action==7?5:2);
                }
                if (effect>=0) gesture_effect_play(effect);
                break;
            }

            case EVENT_BTN_SHORT_PRESS:
                if (1 == event.param1.i32)
                {
                    exit_active_modes();
                    EventBus::publish(EVENT_IMU_SET_MUX,2);
                }
                break;

            case EVENT_IMU_DATA_UPDATED:
                if(!gestureBufferCurrent(event.generation))break;
                // 数据语义由各模式的 mux 决定：3=角速度 / 4=修正角度 / 5=线性加速度
                if (APP_ble_mouse_in_mode())
                {
                    APP_ble_mouse_move(event.param1.f32, event.param2.f32);
                }
                if (APP_ble_volume_in_mode())
                {
                    APP_ble_volume_knob(event.param1.f32);
                }
                if (APP_POV_in_mode())
                {
                    // 剔除重力后的 X-Z 平面线性加速度 (单位 g)，挥动超阈值即触发光绘
                    APP_POV_on_imu_data(event.param1.f32, event.param2.f32);
                }
                break;

            case EVENT_POV_SET:
                // 参数设置事件：修改下一次光绘触发时的图案/方向
                // param1: 图案索引, param2: reverse (0/1)
                APP_POV_set_params((uint8_t)event.param1.i32, event.param2.i32 != 0);
                break;

            default:
                break;
            }
        }
    }
}

void APP_init()
{
    app_queue = xQueueCreate(16, sizeof(SystemEvent)); // 深度16，容纳高频鼠标数据
    EventBus::subscribe(EVENT_GESTURE_DETECTED, app_queue);
    EventBus::subscribe(EVENT_GESTURE_TRIGGER, app_queue);
    EventBus::subscribe(EVENT_GESTURE_FX_PREVIEW, app_queue);
    EventBus::subscribe(EVENT_APP_SET_MODE, app_queue);
    EventBus::subscribe(EVENT_BTN_SHORT_PRESS, app_queue);
    EventBus::subscribe(EVENT_IMU_DATA_UPDATED, app_queue);
    EventBus::subscribe(EVENT_POV_SET, app_queue);

    APP_espnow_init();

    xTaskCreatePinnedToCore(
        app_task,
        "app_task",
        4096,
        NULL,
        2,
        NULL,
        1);
}
