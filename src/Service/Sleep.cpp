/*
    提供进入深度睡眠的功能，提供按键唤醒和IMU中断唤醒两种方式
*/
#include "Sleep.h"
#include <Arduino.h>
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "Config.h"
#include "DualPrint.h"
#include "HAL/HAL.h"

bool sleep_prepare()
{
    // 唤醒源：按键 + IMU 中断（任一低电平即唤醒）
    const uint64_t wakeup_mask = (1ULL << PIN_KEY) | (1ULL << PIN_IMU_INT);
    if (esp_sleep_enable_ext1_wakeup(wakeup_mask, ESP_EXT1_WAKEUP_ANY_LOW) != ESP_OK) return false;
    if (strcmp(HAL::ICM42670P_modelName(), "ICM-42607-P") == 0) {
        const auto pin = static_cast<gpio_num_t>(PIN_IMU_INT);
        if (rtc_gpio_pulldown_dis(pin) != ESP_OK || rtc_gpio_pullup_en(pin) != ESP_OK ||
            esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON) != ESP_OK) return false;
        if (WAND_42607_SLEEP_TRIAL && esp_sleep_enable_timer_wakeup(45ULL * 1000000) != ESP_OK) return false;
    }
    return true;
}

const char *sleep_wake_source()
{
    const auto cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_TIMER) return "timer";
    if (cause != ESP_SLEEP_WAKEUP_EXT1) return "";
    const auto pins = esp_sleep_get_ext1_wakeup_status();
    if (pins & (1ULL << PIN_IMU_INT)) return "motion";
    if (pins & (1ULL << PIN_KEY)) return "button";
    return "";
}

void sleep_enter()
{
    DualSerial.println("[SLEEP] Enter deep sleep");
    delay(100); // 等待串口输出完成

    esp_deep_sleep_start();
}
