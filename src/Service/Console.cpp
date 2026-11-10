/*
    解析串口控制台命令输入，并将命令通过CommandBus发送给Dispatcher处理
*/
#include <Arduino.h>
#include "Console.h"
#include "DualPrint.h"
#include "EventBus.h"
#include "GestureBindings.h"
#include "HomeAssistant.h"
#include "APP/APP_POV.h" // pov 命令的图案清单由 APP 层输出

#define CONSOLE_BUF_SIZE 64

static char rx_buf[CONSOLE_BUF_SIZE];
static uint8_t rx_index = 0;

/************ 显示帮助信息 ************/
static void console_print_help()
{
    DualSerial.println("========== 魔杖使用帮助 / Wand Console ==========");
    DualSerial.println("新手推荐：连接魔杖 Wi-Fi，在浏览器打开 http://192.168.4.1");
    DualSerial.println("help           - 查看帮助");
    DualSerial.println("back           - 退出当前玩法，恢复挥杖识别");
    DualSerial.println("info           - 查看设备与电池信息");
    DualSerial.println("reboot         - 重新启动（网页会暂时断开）");
    DualSerial.println("sleep          - 休眠，拿起可唤醒（网页会断开）");
    DualSerial.println("shutdown       - 关机，单击底部按键开机");
    DualSerial.println("ota            - 固件升级模式，随后打开 /update");
    DualSerial.println("ap             - 切换 Wi-Fi 热点（关闭后网页会断开）");
    DualSerial.println("espnow         - 切换红外配件通信");
    DualSerial.println("ble            - 切换蓝牙开关");
    DualSerial.println("inference      - 恢复挥杖识别，与 back 相同");
    DualSerial.println("imu            - 采集动作数据（暂停施法）");
    DualSerial.println("stop           - 暂停动作检测");
    DualSerial.println("debug          - 保持唤醒，重启恢复自动休眠");
    DualSerial.println("gesture <id>   - 体验功能：4鼠标 5照明 6音量 7光绘 8-11 HA 12-43 红外");
    DualSerial.println("pov [id] [rev] - 查看或选择光绘图案，rev=1 反向播放");
}

/************ 串口命令解析 ************/
void console_parse(char *cmd)
{
    int32_t arg1 = 0, arg2 = 0;

    // 使用 strtok 分割字符串
    char *context=nullptr;
    char *token = strtok_r(cmd, " \r\n", &context); // 第一个单词是命令
    if (token == NULL)
        return;

    DualSerial.print("\n> ");
    DualSerial.println(token);

    if (!strcmp(token, "debug"))
    {
        EventBus::publish(EVENT_SYS_DEBUG);
    }
    else if (!strcmp(token, "info"))
    {
        EventBus::publish(EVENT_SYS_INFO);
    }
    else if (!strcmp(token, "reboot"))
    {
        EventBus::publish(EVENT_SYS_REBOOT);
    }
    else if (!strcmp(token, "sleep"))
    {
        EventBus::publish(EVENT_SYS_SLEEP);
    }
    else if (!strcmp(token, "shutdown"))
    {
        EventBus::publish(EVENT_SYS_SHUTDOWN);
    }

    else if (!strcmp(token, "ota"))
    {
        EventBus::publish(EVENT_SYS_OTA);
    }
    else if (!strcmp(token, "ap"))
    {
        EventBus::publish(EVENT_SYS_AP);
    }
    else if (!strcmp(token, "espnow"))
    {
        EventBus::publish(EVENT_SYS_ESPNOW);
    }
    else if (!strcmp(token, "ble"))
    {
        EventBus::publish(EVENT_SYS_BLE);
    }

    else if (!strcmp(token, "inference") || !strcmp(token, "back"))
    {
        if(!EventBus::publish(EVENT_APP_SET_MODE,2))DualSerial.println("[APP] Mode request busy, retry back");
    }
    else if (!strcmp(token, "imu"))
    {
        EventBus::publish(EVENT_APP_SET_MODE, 1);
    }
    else if (!strcmp(token, "stop"))
    {
        EventBus::publish(EVENT_APP_SET_MODE, -1);
    }

    else if (!strcmp(token, "gesture"))
    {
        token = strtok_r(nullptr, " \r\n", &context); // 第二个单词是参数1
        if (token != NULL)
        {
            char *end = nullptr;
            const long action = strtol(token, &end, 10);
            if (end == token || *end || action < 0 || action > kLastGestureAction || gesture_action_retired(action)) {
                DualSerial.println("Error: unsupported gesture action");
                return;
            }
            arg1 = action;
            EventBus::publish(EVENT_GESTURE_DETECTED, arg1);
            DualSerial.printf("Gesture %d triggered\n", arg1);
        }
        else
        {
            DualSerial.println("Error: gesture command requires an ID argument");
        }
    }

    else if (!strcmp(token, "pov"))
    {
        // 无参数: 由 APP 层打印图案清单与用法
        token = strtok_r(nullptr, " \r\n", &context);
        if (token == NULL)
        {
            APP_POV_list();
            DualSerial.println("Usage: pov <index> [reverse 0/1]");
            DualSerial.println("Note: this command sets params for next trigger");
            return;
        }

        // 索引越界检查与钳制由 APP 层 (APP_POV_set_params) 完成
        arg1 = atoi(token);
        token = strtok_r(nullptr, " \r\n", &context);
        arg2 = (token != NULL) ? atoi(token) : 0; // reverse, 默认 0

        EventBus::publish(EVENT_POV_SET, arg1, arg2);
    }

    else if (!strcmp(token, "help"))
    {
        console_print_help();
        return;
    }
    else
    {
        DualSerial.println("Unknown command");
        return;
    }
}

/************ Console Task ************/
static void console_task(void *param)
{
    while (1)
    {
        while (Serial.available())
        {
            char c = Serial.read();

            // Enter
            if (c == '\n' || c == '\r')
            {
                rx_buf[rx_index] = 0;

                if (rx_index > 0)
                {
                    console_parse(rx_buf);
                }

                rx_index = 0;
            }
            else
            {
                if (rx_index < CONSOLE_BUF_SIZE - 1)
                {
                    rx_buf[rx_index++] = c;
                }
            }
        }

        vTaskDelay(10);
    }
}

/************ Init ************/
void console_init()
{
    xTaskCreatePinnedToCore(
        console_task,
        "console_task",
        4096,
        NULL,
        2,
        NULL,
        0);
}
