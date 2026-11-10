/*
    WiFi射频管理模块：统一管理 ESP32-S3 的 WiFi 射频资源，避免 AP 与 ESP-NOW 之间的冲突
*/
#include "AP.h"
#include "AP_Web.h"
#include "HomeAssistant.h"
#include <atomic>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "IrLink.h"
#include <AsyncTCP.h>
#include "Config.h"
#include "Service/Console.h"
#include "CommandLine.h"
#include "Service/DualPrint.h"

// ==================== 配置与常量定义 ====================
#define TCP_PORT 8080 // TCP 服务器监听端口
#define MAX_CLIENTS 2 // TCP Server 最大连接数

static const uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ==================== 模块内部变量 ====================
static AsyncServer *tcpServer = nullptr;
static AsyncClient *clients[MAX_CLIENTS] = {nullptr};
static CommandLine lines[MAX_CLIENTS];
static SemaphoreHandle_t clientsMutex=nullptr;
static AsyncClient *retired[MAX_CLIENTS]{};
static TaskHandle_t asyncTask=nullptr;

static bool is_ap_enabled = false;
static std::atomic<bool> is_espnow_enabled{false};

// ESP-NOW readiness：init+add_peer 完成后置 true，deinit 前先置 false
static std::atomic<bool> s_espnow_ready{false};

// 已下发到 WiFi 驱动的 mode：用于 update_rf_state() 幂等保护，避免重复 WiFi.mode()
static wifi_mode_t s_current_wifi_mode = WIFI_MODE_NULL;

// ==================== 核心射频(RF)统一调度器 ====================
// AP 开启时 WiFi 锁定 APSTA，ESP-NOW 切换不触发 WiFi.mode()，
// 避免在 NimBLE 运行期间反复拆装 STA 子接口导致共存状态机崩溃。
static void update_rf_state(void)
{
    wifi_mode_t want;
    if (is_ap_enabled)
    {
        want = WIFI_MODE_APSTA;
    }
    else if (is_espnow_enabled || ha_uses_wifi())
    {
        want = WIFI_MODE_STA;
    }
    else
    {
        want = WIFI_MODE_NULL;
    }

    // 幂等保护：避免重复触发 WiFi.mode()
    if (want == s_current_wifi_mode)
    {
        return;
    }
    s_current_wifi_mode = want;

    switch (want)
    {
    case WIFI_MODE_APSTA:
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(AP_SSID, AP_PASS);
        DualSerial.println("[AP] RF Mode -> WIFI_AP_STA");
        break;
    case WIFI_MODE_STA:
        WiFi.mode(WIFI_STA);
        DualSerial.println("[AP] RF Mode -> WIFI_STA");
        break;
    default:
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        DualSerial.println("[AP] RF Mode -> WIFI_OFF");
        break;
    }
}

// ==================== 1. TCP/IP串口模块实现 ====================
// 回调：当网络收到数据时
static void handleData(void *arg, AsyncClient *client, void *data, size_t len)
{
    if(!clientsMutex||xSemaphoreTakeRecursive(clientsMutex,pdMS_TO_TICKS(20))!=pdTRUE)return;
    for(int i=0;i<MAX_CLIENTS;++i)
        if(clients[i]==client){lines[i].feed(data,len,console_parse);break;}
    xSemaphoreGiveRecursive(clientsMutex);
}
// 回调：当客户端断开连接时
static void handleDisconnect(void *arg, AsyncClient *client)
{
    xSemaphoreTakeRecursive(clientsMutex,portMAX_DELAY);
    for(int i=0;i<MAX_CLIENTS;++i)if(clients[i]==client){clients[i]=nullptr;lines[i].reset();}
    // A service-task close may race AsyncTCP's current receive callback.
    // Reclaim such objects only on a later AsyncTCP callback.
    bool local=xTaskGetCurrentTaskHandle()==asyncTask;
    if(!local)for(auto &slot:retired)if(!slot){slot=client;break;}
    xSemaphoreGiveRecursive(clientsMutex);
    if(local){client->onDisconnect(nullptr);delete client;}
}
// 回调：当有新客户端连接时
static void handleNewClient(void *arg, AsyncClient *client)
{
    asyncTask=xTaskGetCurrentTaskHandle();
    xSemaphoreTakeRecursive(clientsMutex,portMAX_DELAY);
    for(auto &old:retired)if(old){old->onDisconnect(nullptr);delete old;old=nullptr;}
    xSemaphoreGiveRecursive(clientsMutex);
    client->onDisconnect(&handleDisconnect,nullptr);
    xSemaphoreTakeRecursive(clientsMutex,portMAX_DELAY);
    // 寻找空闲的插槽
    int freeIndex = -1;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        // 如果插槽为空，或者原客户端已经断开连接
        if (clients[i] == nullptr || !clients[i]->connected())
        {
            freeIndex = i;
            break;
        }
    }

    // 如果没有空闲插槽，则拒绝连接
    if (freeIndex == -1)
    {
        xSemaphoreGiveRecursive(clientsMutex);
        DualSerial.println("[AP] TCP Connection limit reached, rejecting new connection.");
        client->close();
        return;
    }

    // 保存新客户端到空闲插槽
    clients[freeIndex] = client;
    lines[freeIndex].reset();

    // 为这个新客户端注册回调
    client->onData(&handleData, nullptr);
    xSemaphoreGiveRecursive(clientsMutex);

    DualSerial.printf("[AP] TCP Client connected to slot [%d]\n", freeIndex);
}

static void start_ap_server(void)
{
    if(!clientsMutex)clientsMutex=xSemaphoreCreateRecursiveMutex();
    if(!clientsMutex)return;
    is_ap_enabled = true;
    update_rf_state(); // 统一刷新底层 RF 模式

    const auto apAddress = WiFi.softAPIP();
    if (!apAddress) {
        is_ap_enabled = false;
        update_rf_state();
        return;
    }
    if (tcpServer == nullptr)
    {
        tcpServer = new AsyncServer(apAddress, TCP_PORT);
    }

    // 注册新客户端连接的回调函数
    tcpServer->onClient(&handleNewClient, nullptr);
    tcpServer->begin();

    DualSerial.println("[AP] TCP Server Started");
    DualSerial.print("[AP] IP Address: ");
    DualSerial.println(WiFi.softAPIP());
    ap_web_start(); // 同时启动 Web 控制面板
    ha_start();
}

static void stop_ap_server(void)
{
    xSemaphoreTakeRecursive(clientsMutex,portMAX_DELAY);
    // 关闭所有在线的客户端
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] && clients[i]->connected())
        {
            clients[i]->close();
        }
        clients[i] = nullptr;
    }

    xSemaphoreGiveRecursive(clientsMutex);
    if (tcpServer)
    {
        tcpServer->end();
    }

    is_ap_enabled = false;
    update_rf_state(); // 如果 ESP-NOW 没开，此时会自动 WiFi.mode(WIFI_OFF)
    DualSerial.println("[AP] TCP Server Stopped");
    ap_web_stop(); // 同时停止 Web 控制面板
}

void ap_toggle(void)
{
    if (is_ap_enabled)
        stop_ap_server();
    else
        start_ap_server();
}

bool ap_is_running(void)
{
    return is_ap_enabled;
}

void ap_print(const char *buffer, size_t length)
{
    if (!is_ap_enabled)
        return;

    // Never stall the IMU on a slow/disconnecting network client.
    if(clientsMutex&&xSemaphoreTakeRecursive(clientsMutex,0)==pdTRUE){
        for(auto *client:clients)if(client&&client->connected()&&client->space()>=length){
            client->add(buffer,length);client->send();
        }
        xSemaphoreGiveRecursive(clientsMutex);
    }

    ap_web_print(buffer, length); // 同时广播到 WebSocket 客户端
}

// ==================== 2. ESP-NOW 模块实现 ====================
static void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {}
static void OnDataRecv(const uint8_t *mac, const uint8_t *data, int length) { ir_link_receive(mac, data, length); }

static void start_espnow(void)
{
    is_espnow_enabled = true;
    update_rf_state();

    if (esp_now_init() != ESP_OK)
    {
        DualSerial.println("[AP] ESP-NOW Init Failed!");
        is_espnow_enabled = false; // init 失败回滚 flag
        update_rf_state();
        return;
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    // 添加广播 Target
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastMac, 6);
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    s_espnow_ready = true;
    DualSerial.println("[AP] ESP-NOW Started");
}

static void stop_espnow(void)
{
    s_espnow_ready = false;
    is_espnow_enabled = false;

    esp_now_deinit();
    update_rf_state();

    DualSerial.println("[AP] ESP-NOW Stopped");
}

void espnow_toggle(void)
{
    if (is_espnow_enabled)
        stop_espnow();
    else
        start_espnow();
}

bool espnow_channel_conflict()
{
    return is_espnow_enabled && WiFi.channel() != 1;
}

bool espnow_is_running(void)
{
    return is_espnow_enabled;
}

// 通用底层发送接口：把具体的数据内容与长度完全解耦给外部
bool espnow_send_data(const uint8_t *data, size_t len)
{
    if (!s_espnow_ready || espnow_channel_conflict() || data == nullptr || len == 0)
        return false;

    return (esp_now_send(broadcastMac, data, len) == ESP_OK);
}

bool espnow_send_to(const uint8_t mac[6], const uint8_t *data, size_t len)
{
    if (!s_espnow_ready || espnow_channel_conflict() || !mac || !data || !len) return false;
    if (!esp_now_is_peer_exist(mac)) {
        esp_now_peer_info_t peer = {};
        memcpy(peer.peer_addr, mac, 6);
        peer.channel = 0;
        peer.encrypt = false;
        if (esp_now_add_peer(&peer) != ESP_OK) return false;
    }
    return esp_now_send(mac, data, len) == ESP_OK;
}
