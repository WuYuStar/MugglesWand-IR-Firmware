/*
    AP Web Dashboard — 为AP模式提供网页控制面板
    功能：新手引导、玩法体验、真实设备状态、练习反馈和高级诊断
*/
#include "AP_Web.h"
#include "Config.h"
#include "Service/Console.h"
#include "Service/DualPrint.h"
#include "Service/EventBus.h"
#include "Service/GestureBindings.h"
#include "Service/GestureEffects.h"
#include "Model/ModelStore.h"
#include "Model/gesture_inference.h"
#include "ModelHttp.h"
#include "HomeAssistant.h"
#include "IrLink.h"
#include <esp_heap_caps.h>
#include <WiFi.h>
#include <atomic>
#include <ESPAsyncWebServer.h>

#include "WebPage.h"
#include "Service/AP.h"
#include "Service/BLE.h"
#include "HAL/HAL.h"
#include "APP/APP_POV.h"

// ==================== Web 服务模块 ====================
// 使用静态全局对象，避免 new/delete 导致的生命周期问题（AsyncTCP 回调竞争）
static AsyncWebServer webServer(80);
static AsyncWebSocket webSocket("/ws");
static std::atomic<uint32_t> webClients{0};
static std::atomic<bool> webRunning{false};
static bool webRoutesSet = false;

// Escape configurable device names before embedding them in a JSON message.
static String jsonString(const char *value)
{
    String result = "\"";
    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(value); *p; ++p)
    {
        if (*p == '"' || *p == '\\')
        {
            result += '\\';
            result += static_cast<char>(*p);
        }
        else if (*p < 0x20)
        {
            char escaped[7];
            snprintf(escaped, sizeof(escaped), "\\u%04x", *p);
            result += escaped;
        }
        else
            result += static_cast<char>(*p);
    }
    result += '"';
    return result;
}

bool ap_web_local(AsyncWebServerRequest *request)
{
    const IPAddress local = WiFi.softAPIP();
    return local != IPAddress() && request->client()->localIP() == local;
}

static bool localWebSocket(AsyncWebServerRequest *request)
{
    if (!ap_web_local(request)) return false;
    const auto *origin = request->getHeader("Origin");
    return !origin || origin->value() == ("http://" + WiFi.softAPIP().toString());
}

static void sendWebStatus(AsyncWebSocketClient *client)
{
    // Only read cached state here: no I2C transactions in AsyncTCP callbacks.
    String message;
    message.reserve(2600);
    message = "{\"type\":\"status\",\"version\":";
    message += jsonString(FIRMWARE_VER);
    message += ",\"ssid\":" + jsonString(AP_SSID);
    message += ",\"password\":" + jsonString(AP_PASS);
    message += ",\"bleName\":" + jsonString(BLE_DEVICE_NAME);
    message += ",\"mode\":" + String(HAL::ICM42670P_getDataMux());
    message += ",\"bleEnabled\":";
    message += ble_is_enabled() ? "true" : "false";
    message += ",\"bleConnected\":";
    message += ble_is_connected() ? "true" : "false";
    message += ",\"charging\":";
    message += HAL::power_getChargeStatus() ? "true" : "false";
    message += ",\"receiver\":";
    message += espnow_is_running() ? "true" : "false";
    message += ",\"receiverConflict\":";
    message += espnow_channel_conflict() ? "true" : "false";
    message += ",\"irSlots\":" + ir_link_json();
    message += ",\"irLink\":" + ir_link_connection_json();
    message += ",\"haState\":" + String(static_cast<int>(ha_state()));
    message += ",\"internalFree\":" + String(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    message += ",\"internalMin\":" + String(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    message += ",\"internalLargest\":" + String(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    message += ",\"patterns\":[";
    for (uint8_t i = 0; i < APP_POV_pattern_count(); ++i)
    {
        if (i) message += ',';
        message += jsonString(APP_POV_pattern_name(i));
    }
    message += "],\"gestureProfile\":" + jsonString(gesture_model_key());
    message += ",\"gestureProfileTitle\":" + jsonString(gesture_model_title());
    message += ",\"imuReady\":";message+=HAL::ICM42670P_ready()?"true":"false";
    message += ",\"inferenceReady\":";message+=inference_ready()?"true":"false";
    message += ",\"sampleCount\":"+String(HAL::ICM42670P_samples());
    message += ",\"sampleAgeMs\":"+String(HAL::ICM42670P_sampleAge());
    message += ",\"imuRecoveries\":"+String(HAL::ICM42670P_recoveries());
    message += ",\"uptimeMs\":"+String(millis());
    message += ",\"freeHeap\":"+String(ESP.getFreeHeap());
    message += ",\"minFreeHeap\":"+String(ESP.getMinFreeHeap());
    auto bindings = gesture_bindings_state();
    message += ",\"bindingRevision\":" + String(bindings.revision);
    message += ",\"bindingsWritable\":";
    message += bindings.writable ? "true" : "false";
    message += ",\"gestureLabels\":[";
    for (int i = 0; i < gesture_model_count(); ++i) {
        if (i) message += ',';
        message += jsonString(gesture_model_label(i));
    }
    message += "],\"bindings\":[";
    for (int i = 0; i < gesture_model_count(); ++i) {
        if (i) message += ',';
        message += String(bindings.actions[i]);
    }
    message += "],\"gestureEffects\":[";
    for (int i=0;i<gesture_model_count();++i) {if(i)message+=',';message+=String(bindings.effects[i]);}
    message += "],\"modelUpdating\":";message+=ModelStore::busy()?"true":"false";
    message += ",\"lightEffects\":[";
    for (int i=0;i<kGestureEffectChoiceCount;++i) {
        if(i)message+=',';
        const auto &fx=kGestureEffects[i];
        message += "{\"id\":"+String(fx.id)+",\"name\":"+jsonString(fx.name)+",\"group\":"+jsonString(fx.group)+"}";
    }
    message += "]}";
    client->text(message);
}

static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                       AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        ++webClients;
        DualSerial.printf("[Web] Client #%u connected\n", client->id());
        break;

    case WS_EVT_DISCONNECT:
        if(webClients)--webClients;
        DualSerial.printf("[Web] Client #%u disconnected\n", client->id());
        break;

    case WS_EVT_DATA:
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len &&
            info->opcode == WS_TEXT)
        {
            constexpr char haPrefix[] = "web:ha-config\n";
            if (len >= sizeof(haPrefix)-1 && !memcmp(data,haPrefix,sizeof(haPrefix)-1)) {
                char fields[320];
                const size_t size = len - (sizeof(haPrefix)-1);
                HaSave result = HaSave::Invalid;
                if (!ModelStore::busy() && size < sizeof(fields) && !memchr(data+sizeof(haPrefix)-1,'\0',size)) {
                    memcpy(fields,data+sizeof(haPrefix)-1,size);
                    fields[size] = '\0';
                    result = ha_save(fields);
                }
                if (result == HaSave::Saved) client->text("{\"type\":\"haConfig\",\"ok\":true}");
                else if (result == HaSave::StorageError) client->text("{\"type\":\"haConfig\",\"ok\":false,\"message\":\"保存失败，原配置仍保留\"}");
                else client->text("{\"type\":\"haConfig\",\"ok\":false,\"message\":\"请检查配置或等待模型更新结束\"}");
                break;
            }
            // AsyncTCP owns data; never write a terminator past its frame buffer.
            // Bound mapping messages and reject binary NULs.
            if (len == 0 || len >= 128 || memchr(data, '\0', len) != nullptr)
                return;
            char cmd[128];
            memcpy(cmd, data, len);
            cmd[len] = '\0';
            if (strcmp(cmd, "web:status") == 0)
                sendWebStatus(client);
            else if(ModelStore::busy())client->text("{\"type\":\"busy\",\"message\":\"正在更新模型，请稍后再操作\"}");
            else if (strncmp(cmd, "web:ir-pair ", 12) == 0) {
                bool ok = ir_link_pair(cmd + 12);
                client->text(String("{\"type\":\"irPair\",\"ok\":") + (ok ? "true" : "false") + "}");
                if (ok) ir_link_command(0x40, 0);
                sendWebStatus(client);
            }
            else if (strcmp(cmd, "web:ir-unpair") == 0) {
                bool ok = ir_link_unpair();
                client->text(String("{\"type\":\"irPair\",\"ok\":") + (ok ? "true" : "false") + "}");
                sendWebStatus(client);
            }
            else if (strncmp(cmd, "web:ir ", 7) == 0) {
                unsigned type, slot; char extra;
                bool parsed = sscanf(cmd + 7, "%u %u %c", &type, &slot, &extra) == 2;
                bool ok = parsed && ir_link_command(type, slot);
                client->text(String("{\"type\":\"ir\",\"ok\":") + (ok ? "true" : "false") + "}");
            }
            else if (strncmp(cmd, "web:ir-name ", 12) == 0) {
                const char *p = cmd + 12;
                unsigned slot; int consumed = 0;
                if (sscanf(p, "%u %n", &slot, &consumed) != 1 || slot >= 32 || consumed == 0 || !p[consumed]) {
                    client->text("{\"type\":\"ir\",\"ok\":false}");
                } else {
                    bool ok = ir_link_command(0x44, static_cast<uint8_t>(slot), p + consumed);
                    client->text(String("{\"type\":\"ir\",\"ok\":") + (ok ? "true" : "false") + "}");
                }
            }
            else if (strcmp(cmd, "web:ha-test") == 0) {
                if (!ha_send(-1)) client->text("{\"type\":\"haTest\",\"ok\":false}");
            }
            else if (strncmp(cmd, "web:bindings", 12) == 0) {
                int8_t actions[kMaxGestureClasses],effects[kMaxGestureClasses]; uint32_t revision;
                BindingSave result = gesture_bindings_parse(cmd, actions, effects, revision)
                    ? gesture_bindings_save(actions, effects, revision) : BindingSave::Invalid;
                const char *message = result == BindingSave::Saved ? "已保存，立即生效，断电后保留。" :
                    result == BindingSave::Conflict ? "配置已在其他页面改变，请重新读取后再修改。" :
                    result == BindingSave::Invalid ? "配置格式无效，没有修改。" : "保存失败，原配置保留，请重试。";
                String reply = "{\"type\":\"bindings\",\"ok\":";
                reply += result == BindingSave::Saved ? "true" : "false";
                reply += ",\"message\":" + jsonString(message) + "}";
                client->text(reply);
                sendWebStatus(client);
            }
            else if (strncmp(cmd,"web:effect ",11)==0) {
                const char *p=cmd+11;
                int id=0;bool valid=*p>='1' && *p<='9';
                while (*p>='0' && *p<='9' && id<=kGestureEffectMax) id=id*10+*p++-'0';
                valid=valid && !*p && id>=1 && id<=kGestureEffectMax;
                bool queued=valid && EventBus::publish(EVENT_GESTURE_FX_PREVIEW,id);
                String reply="{\"type\":\"effect\",\"ok\":";
                reply+=queued?"true":"false";
                reply+=",\"message\":"+jsonString(queued?"灯效预览已发送，不执行功能。":"无法预览，请稍后重试。")+"}";
                client->text(reply);
            }
            else if (len < 64)
                console_parse(cmd);
        }
        break;
    }

    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

void ap_web_start(void)
{
    if (webRunning)
        return;

    if (!webRoutesSet)
    {
        webSocket.onEvent(onWsEvent);
        webSocket.setFilter(localWebSocket);
        webServer.addHandler(&webSocket);
        model_http_routes(webServer);

        // 根路径：返回内嵌网页
        webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
                          auto *response = request->beginResponse_P(200, "text/html; charset=utf-8", WEB_PAGE);
                          response->addHeader("Cache-Control", "no-store");
                          request->send(response);
                      }).setFilter(ap_web_local);

        webRoutesSet = true;
    }

    webServer.begin();
    webRunning = true;
    DualSerial.println("[Web] HTTP + WebSocket server started on port 80");
}

void ap_web_stop(void)
{
    if (!webRunning)
        return;

    webSocket.closeAll();
    webServer.end();
    webRunning = false;
    DualSerial.println("[Web] Server stopped");
}

void ap_web_print(const char *buffer, size_t length)
{
    if (!webRunning)
        return;

    // WebSocket textAll 内部通过 AsyncTCP 排队发送，跨任务安全
    webSocket.textAll(buffer, length);
}

void ap_web_gesture_result(int id, float confidence, bool accepted, int action, int effect)
{
    if (!webRunning) return;
    char message[192];
    snprintf(message, sizeof(message),
             "{\"type\":\"recognition\",\"id\":%d,\"action\":%d,\"effect\":%d,\"confidence\":%.4f,\"accepted\":%s}",
             id, action, effect, confidence, accepted ? "true" : "false");
    webSocket.textAll(message);
}

bool ap_web_has_clients(){return webClients>0;}
