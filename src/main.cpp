#include <Arduino.h>

#include "Service/DualPrint.h"
#include "Service/EventBus.h"
#include "Service/Console.h"
#include "Service/Service.h"
#include "Service/GestureBindings.h"
#include "Service/IrLink.h"
#include "Service/HomeAssistant.h"
#include "Model/ModelStore.h"

#include "HAL/HAL.h"

#include "Model/gesture_inference.h"

#include "APP/APP.h"
#include "HAL/WS2812_Animation/AnimBootup.hpp"

/*
Core 0:
| Task              | Priority
|-------------------|----------
| Wi-Fi/BLE stack   | highest(default)
| OTA               | 4
| Service           | 3
| Console parser    | 2
| Button handling   | 1
| BLE battery task  | 0
| Power task        | 0

Core 1:
| Task              | Priority
|-------------------|----------
| WS2812 animation  | 4
| IMU sampling      | 3
| APP task          | 2
| Gesture reference | 1
| ESP-NOW heartbeat | 1
*/

void setup()
{
  // ---------------系统初始化---------------
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0); // A disconnected USB monitor must not stall sampling.

  DualSerial.println("[System] Initializing...");

  // 系统服务初始化
  EventBus::init();
  ModelStore::init();
  ha_init();
  console_init();
  service_init();

  // 模型载入
  inference_init();
  gesture_bindings_init();
  ir_link_init();

  // 硬件初始化
  HAL::power_init();
  HAL::ws2812_init();
  // 开机/唤醒动画：中心向两端扩散后整体熄灭，播完自销毁
  HAL::ws2812_start_fx(new AnimBootup(CRGB::White, 0, HAL::ws2812_count()));
  bool imu_ok = HAL::ICM42670P_init();
  bool batt_ok = HAL::MAX17048_init();
  HAL::button_init();

  // 用户APP初始化
  APP_init();

  // 启动手势推理、BLE、WiFi热点
  if(imu_ok){inference_start();EventBus::publish(EVENT_IMU_SET_MUX,2);}
  EventBus::publish(EVENT_SYS_AP);
  EventBus::publish(EVENT_SYS_BLE);

  if(!imu_ok)DualSerial.println("[System] IMU unavailable; Wi-Fi remains available for recovery");
  if(!batt_ok)DualSerial.println("[System] Battery gauge unavailable; gesture functions remain available");

  DualSerial.println("[System] Initialization complete");
}

void loop()
{
}
