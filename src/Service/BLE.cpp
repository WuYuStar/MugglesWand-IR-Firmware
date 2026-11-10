#include "BLE.h"
#include <NimBLEDevice.h>
#include <math.h>
#include <NimBLEHIDDevice.h>
#include "HAL/HAL.h"
#include "Config.h"

// 全局静态变量管理内部状态
static String _device_name = BLE_DEVICE_NAME;
static volatile bool _is_ble_enabled = false;
static volatile bool _is_connected = false;

// FreeRTOS 任务句柄与退出标志
static TaskHandle_t _battery_task_handle = nullptr;

// NimBLE 核心指针
static NimBLEServer *pServer = nullptr;

// HID 核心指针与特征值
// Report ID 2: Mouse   |   Report ID 3: Consumer Control
static NimBLEHIDDevice *pHID = nullptr;
static NimBLECharacteristic *pMouseInput     = nullptr; // 鼠标输入 (Report ID 2)
static NimBLECharacteristic *pConsumerInput  = nullptr; // 消费类控制输入：音量、播放等媒体键 (Report ID 3)

// ========== 电池 CCCD 订阅回调 ==========
// 等主机完成 CCCD 订阅后，立即发送一次电量通知
class BatteryCallbacks : public NimBLECharacteristicCallbacks
{
    void onSubscribe(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo, uint16_t subValue) override
    {
        if (subValue == 1)
        {
            ble_update_battery();
        }
        else
        {
            DualSerial.println("[ERR] Battery notifications unsubscribed");
        }
    }
};

// 复合 HID 报告描述符：Mouse (ID 2) + Consumer Control (ID 3)
// 各 Report ID 独立，主机端根据 ID 将数据分发到对应驱动子系统
static const uint8_t _hidReportDescriptor[] = {
    // ==================== Report ID 2: Mouse ====================
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x02, // Usage (Mouse)
    0xA1, 0x01, // Collection (Application)
    0x85, 0x02, //   Report ID (2)
    0x09, 0x01, //   Usage (Pointer)
    0xA1, 0x00, //   Collection (Physical)

    // 3 个按钮
    0x05, 0x09, //     Usage Page (Buttons)
    0x19, 0x01, //     Usage Minimum (Button 1)
    0x29, 0x03, //     Usage Maximum (Button 3)
    0x15, 0x00, //     Logical Minimum (0)
    0x25, 0x01, //     Logical Maximum (1)
    0x95, 0x03, //     Report Count (3)
    0x75, 0x01, //     Report Size (1)
    0x81, 0x02, //     Input (Data, Variable, Absolute)

    // 5 bit 填充
    0x95, 0x01, //     Report Count (1)
    0x75, 0x05, //     Report Size (5)
    0x81, 0x01, //     Input (Constant)

    // X, Y, Wheel（相对位移）
    0x05, 0x01, //     Usage Page (Generic Desktop)
    0x09, 0x30, //     Usage (X)
    0x09, 0x31, //     Usage (Y)
    0x09, 0x38, //     Usage (Wheel)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum (127)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x03, //     Report Count (3)
    0x81, 0x06, //     Input (Data, Variable, Relative)

    0xC0, //   End Collection (Physical)
    0xC0, // End Collection (Mouse)

    // ==================== Report ID 3: Consumer Control ====================
    // 用于发送媒体键（音量 +/-、播放/暂停、静音等），usage code 定义在 BLEHIDKeys.h
    // 报文格式：2 字节小端序 usage code，发送后紧跟全零报文释放
    0x05, 0x0C,       // Usage Page (Consumer Devices)
    0x09, 0x01,       // Usage (Consumer Control)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x03,       //   Report ID (3)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xFF, 0x03, //   Logical Maximum (1023)
    0x19, 0x00,       //   Usage Minimum (0)
    0x2A, 0xFF, 0x03, //   Usage Maximum (1023)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x00,       //   Input (Data, Array, Absolute)
    0xC0,   // End Collection (Consumer Control)
};

/**
 * @brief 服务器回调类，用于监听连接和断开事件
 */
class MyServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
    {
        _is_connected = true;
        // 允许连接后更新参数以优化功耗和速度（可选）
        // 低延迟连接参数：min=7.5ms, max=12.5ms, latency=0, timeout=1000ms
        // 为鼠标模式提供 ~80-133Hz 报告速率
        pServer->updateConnParams(connInfo.getConnHandle(), 6, 10, 0, 100);
        DualSerial.printf("[BLE] Client connected\n");
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
    {
        _is_connected = false;
        DualSerial.printf("[BLE] Client disconnected\n");
        // 注意：如果 BLE 被主动 toggle 关闭，不需要在这里重新开启广播
        if (_is_ble_enabled)
        {
            NimBLEDevice::startAdvertising();
        }
    }
};

bool ble_update_battery(void)
{
    if (!_is_ble_enabled || !_is_connected || pHID == nullptr)
    {
        return false;
    }

    float level = (HAL::MAX17048_getSOC() + 0.5f); // 加0.5为了取整时四舍五入
    if(!isfinite(level))return false;
    level=constrain(level,0.0f,100.0f); // 限制最大值为 100

    pHID->setBatteryLevel((uint8_t)level, 1);

    return true;
}

/**
 * @brief 电池电量定期上报的 FreeRTOS 任务函数
 * @details 对象和任务在本次开机期间保留，关闭蓝牙时不发送通知。
 */
static void ble_battery_task(void *pvParameters)
{
    for(;;){ble_update_battery();vTaskDelay(pdMS_TO_TICKS(10000));}
}

// 发送鼠标相对位移报文
// dx, dy: 有符号 8-bit 位移 (-127 ~ 127)
bool ble_mouse_move(int8_t dx, int8_t dy)
{
    if (!_is_ble_enabled || !_is_connected || pMouseInput == nullptr)
    {
        return false;
    }

    // Mouse HID Report (Report ID 2): [buttons(1) | X(1) | Y(1) | Wheel(1)]
    uint8_t report[4] = {0};
    report[1] = (uint8_t)dx;
    report[2] = (uint8_t)dy;
    // report[0] = 0 (no buttons pressed)
    // report[3] = 0 (no wheel)

    pMouseInput->setValue(report, sizeof(report));
    pMouseInput->notify();

    return true;
}

bool ble_toggle(void)
{
    if(!pServer){
        NimBLEDevice::init(_device_name.c_str());
        NimBLEDevice::setSecurityAuth(true,true,true);
        pServer=NimBLEDevice::createServer();
        pServer->setCallbacks(new MyServerCallbacks());
        pHID=new NimBLEHIDDevice(pServer);
        pHID->setReportMap((uint8_t*)_hidReportDescriptor,sizeof(_hidReportDescriptor));
        pHID->setPnp(0x02,0x05ac,0x0255,0x0110);pHID->setHidInfo(0,1);
        pMouseInput=pHID->getInputReport(2);pConsumerInput=pHID->getInputReport(3);
        pHID->getBatteryLevel()->setCallbacks(new BatteryCallbacks());
        auto *advertising=NimBLEDevice::getAdvertising();
        advertising->setName(_device_name.c_str());
        advertising->addServiceUUID(pHID->getHidService()->getUUID());
        advertising->setAppearance(0x03C4);advertising->enableScanResponse(true);
        xTaskCreatePinnedToCore(ble_battery_task,"ble_bat_task",4096,nullptr,1,&_battery_task_handle,0);
    }
    _is_ble_enabled=!_is_ble_enabled;
    if(_is_ble_enabled)NimBLEDevice::startAdvertising();
    else{
        _is_connected=false;NimBLEDevice::getAdvertising()->stop();
        for(auto peer:pServer->getPeerDevices())pServer->disconnect(peer);
    }
    // Keep HID objects alive: APP notifications and battery callbacks may still
    // be in flight on the other core when the user switches Bluetooth off.
    DualSerial.println(_is_ble_enabled?"[BLE] Enabled":"[BLE] Disabled");
    return _is_ble_enabled;
}

/**
 * @brief 发送 HID Consumer Control 按键（媒体键）
 *
 * @details Consumer Control 采用"按下即释放"模式：先发送目标 usage code，
 *          再紧跟全零报文释放，避免主机端将按键卡住（stuck key）。
 *          此模式适用于音量、播放控制等无需保持按下的瞬时操作。
 *
 * @param usage_code Consumer Page (0x0C) usage code，使用 BLEHIDKeys.h 中的
 *                    MEDIA_* 宏（如 MEDIA_VOLUME_UP = 0x00E9）
 * @return true 发送成功，false BLE 未开启、未连接或 Consumer 特征值未初始化
 */
bool ble_consumer_send(uint16_t usage_code)
{
    if (!_is_ble_enabled || !_is_connected || pConsumerInput == nullptr)
    {
        return false;
    }

    // Consumer HID Report (Report ID 3): 2 字节小端序 usage code
    //   byte[0] = usage_code 低 8 位
    //   byte[1] = usage_code 高 8 位
    uint8_t report[2] = {
        (uint8_t)(usage_code & 0xFF),
        (uint8_t)((usage_code >> 8) & 0xFF)
    };

    // 1. 发送目标按键
    pConsumerInput->setValue(report, sizeof(report));
    pConsumerInput->notify();

    // 2. 立即发送全零报文释放按键，避免主机认为按键一直处于按下状态
    uint8_t release[2] = {0, 0};
    pConsumerInput->setValue(release, sizeof(release));
    pConsumerInput->notify();

    return true;
}

bool ble_is_enabled(void) { return _is_ble_enabled; }
bool ble_is_connected(void) { return _is_connected; }
