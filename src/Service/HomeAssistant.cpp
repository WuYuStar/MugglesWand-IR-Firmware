#include "HomeAssistant.h"
#include "AP_Web.h"
#include "OTA.h"

#include <Arduino.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <atomic>
#include <esp_mac.h>

namespace {
struct Config {
    uint8_t version = 1;
    uint8_t enabled = 0;
    uint16_t port = 1883;
    char ssid[33]{}, wifiKey[64]{}, broker[64]{}, user[32]{}, mqttKey[64]{};
};

struct Event {
    int action;
    uint32_t queuedAt;
    uint32_t session;
};

Config active;
QueueHandle_t events = nullptr;
std::atomic<HaState> state{HaState::Disabled};
std::atomic<uint32_t> session{0};
std::atomic<bool> started{false};

bool valid(const Config &c)
{
    if (c.version != 1 || c.enabled > 1 || !c.port) return false;
    const size_t wifiLength = strnlen(c.wifiKey, sizeof(c.wifiKey));
    if (strnlen(c.ssid, sizeof(c.ssid)) == sizeof(c.ssid) ||
        wifiLength == sizeof(c.wifiKey) ||
        strnlen(c.broker, sizeof(c.broker)) == sizeof(c.broker) ||
        strnlen(c.user, sizeof(c.user)) == sizeof(c.user) ||
        strnlen(c.mqttKey, sizeof(c.mqttKey)) == sizeof(c.mqttKey)) return false;
    if (!c.enabled) return true;
    if (!c.ssid[0] || !c.broker[0] || (wifiLength && wifiLength < 8) ||
        (!c.user[0] && c.mqttKey[0])) return false;
    for (const char *p = c.broker; *p; ++p) {
        if (!isalnum(static_cast<unsigned char>(*p)) && *p != '-' && *p != '.') return false;
    }
    return true;
}

bool copyField(char *target, size_t capacity, const char *value)
{
    if (strlen(value) >= capacity) return false;
    for (const char *p = value; *p; ++p) {
        if (static_cast<unsigned char>(*p) < 32 || *p == 127) return false;
    }
    strcpy(target, value);
    return true;
}

void testResult(bool sent)
{
    const char *result = sent ? "{\"type\":\"haTest\",\"ok\":true}" : "{\"type\":\"haTest\",\"ok\":false}";
    ap_web_print(result, strlen(result));
}

void runMqtt()
{
    if (OTA_active()) return;
    WiFiClient socket;
    PubSubClient mqtt(socket);
    uint8_t mac[6];
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK) return;
    char id[24];
    snprintf(id, sizeof(id), "wand_%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    char eventTopic[48], availability[48], discoveryTopic[64], discovery[460];
    snprintf(eventTopic, sizeof(eventTopic), "muggleswand/%s/event", id);
    snprintf(availability, sizeof(availability), "muggleswand/%s/availability", id);
    snprintf(discoveryTopic, sizeof(discoveryTopic), "homeassistant/event/%s/gesture/config", id);
    snprintf(discovery, sizeof(discovery),
             "{\"name\":\"Gesture\",\"unique_id\":\"%s_gesture\",\"state_topic\":\"%s\","
             "\"availability_topic\":\"%s\",\"event_types\":[\"scene_1\",\"scene_2\",\"scene_3\",\"scene_4\",\"test\"],"
             "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"Muggles' Wand\"}}", id, eventTopic, availability, id);
    if (!mqtt.setBufferSize(640)) return;
    mqtt.setServer(active.broker, active.port);
    mqtt.setSocketTimeout(2);
    mqtt.setKeepAlive(15);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.begin(active.ssid, active.wifiKey);

    uint32_t retryAt = millis(), backoff = 1000;
    while (!OTA_active()) {
        if (WiFi.status() != WL_CONNECTED) {
            state = HaState::ConnectingWifi;
            socket.stop();
            retryAt = millis();
            backoff = 1000;
        } else if (state != HaState::Ready || !mqtt.loop()) {
            state = HaState::ConnectingMqtt;
            const uint32_t now = millis();
            if (static_cast<int32_t>(now - retryAt) >= 0) {
                socket.stop();
                mqtt.connected(); // Clear the previous MQTT session before opening a new TCP socket.
                if (socket.connect(active.broker, active.port, 800) &&
                    mqtt.connect(id, active.user[0] ? active.user : nullptr,
                                 active.user[0] ? active.mqttKey : nullptr,
                                 availability, 1, true, "offline")) {
                    state = HaState::Discovering;
                    if (!OTA_active() && mqtt.publish(discoveryTopic, discovery, true) &&
                        mqtt.publish(availability, "online", true)) {
                        ++session;
                        state = HaState::Ready;
                        backoff = 1000;
                    }
                }
                retryAt = millis() + backoff;
                if (state != HaState::Ready) {
                    state = HaState::ConnectingMqtt;
                    socket.stop();
                    backoff = min(backoff * 2, uint32_t(30000));
                }
            }
        }

        Event event;
        if (xQueueReceive(events, &event, pdMS_TO_TICKS(50)) == pdTRUE) {
            bool sent = false;
            if (!OTA_active() && state == HaState::Ready && event.session == session &&
                WiFi.status() == WL_CONNECTED && millis() - event.queuedAt <= 750) {
                char payload[32];
                if (event.action == -1) strcpy(payload, "{\"event_type\":\"test\"}");
                else snprintf(payload, sizeof(payload), "{\"event_type\":\"scene_%d\"}", event.action - kFirstHaAction + 1);
                sent = mqtt.publish(eventTopic, payload, false);
                if (!sent) {
                    state = HaState::ConnectingMqtt;
                    socket.stop();
                }
            }
            if (event.action == -1) testResult(sent);
        }
    }
    socket.stop(); // Broker publishes the last will when the session closes.
}

void worker(void *)
{
    runMqtt();
    state = HaState::Unavailable;
    vTaskDelete(nullptr);
}
}

void ha_init()
{
    Preferences prefs;
    Config stored;
    if (prefs.begin("wand-ha", true) && prefs.getBytesLength("config") == sizeof(stored) &&
        prefs.getBytes("config", &stored, sizeof(stored)) == sizeof(stored) && valid(stored)) {
        active = stored;
    }
    state = active.enabled ? HaState::ConnectingWifi : HaState::Disabled;
}

void ha_start()
{
    if (!active.enabled || OTA_active() || started.exchange(true)) return;
    events = xQueueCreate(6, sizeof(Event));
    if (!events || xTaskCreatePinnedToCore(worker, "wand-ha", 6144, nullptr, 1, nullptr, 0) != pdPASS) {
        if (events) vQueueDelete(events);
        events = nullptr;
        state = HaState::Unavailable;
    }
}

bool ha_uses_wifi() { return active.enabled; }
HaState ha_state() { return state; }

HaSave ha_save(char *fields)
{
    char *values[7] = {fields};
    for (size_t i = 1; i < 7; ++i) {
        char *separator = strchr(values[i-1], '\n');
        if (!separator) return HaSave::Invalid;
        *separator = '\0';
        values[i] = separator + 1;
    }
    if (strchr(values[6], '\n') || (strcmp(values[0], "0") && strcmp(values[0], "1"))) return HaSave::Invalid;

    Config c;
    c.enabled = values[0][0] == '1';
    if (c.enabled) {
        unsigned port = 0;
        for (const char *p = values[4]; *p; ++p) {
            if (*p < '0' || *p > '9') return HaSave::Invalid;
            port = port * 10 + *p - '0';
            if (port > 65535) return HaSave::Invalid;
        }
        c.port = port;
        if (!copyField(c.ssid, sizeof(c.ssid), values[1]) ||
            !copyField(c.wifiKey, sizeof(c.wifiKey), values[2]) ||
            !copyField(c.broker, sizeof(c.broker), values[3]) ||
            !copyField(c.user, sizeof(c.user), values[5]) ||
            !copyField(c.mqttKey, sizeof(c.mqttKey), values[6]) || !valid(c)) return HaSave::Invalid;
    }
    Preferences prefs;
    if (!prefs.begin("wand-ha", false) || prefs.putBytes("config", &c, sizeof(c)) != sizeof(c))
        return HaSave::StorageError;
    return HaSave::Saved;
}

bool ha_send(int action)
{
    if (action != -1 && (action < kFirstHaAction || action > 11)) return false;
    // Snapshot the session before readiness so a reconnect cannot reclassify an old event.
    const Event event{action, millis(), session.load()};
    return state == HaState::Ready && !OTA_active() && xQueueSend(events, &event, 0) == pdTRUE;
}
