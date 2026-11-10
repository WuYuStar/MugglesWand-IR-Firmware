#pragma once

#include <stdint.h>

constexpr int kFirstHaAction = 8;
constexpr int kLastGestureAction = 43; // 12..43 address 32 IR signals.

enum class HaState : uint8_t { Disabled, ConnectingWifi, ConnectingMqtt, Discovering, Ready, Unavailable };
enum class HaSave : uint8_t { Saved, Invalid, StorageError };

void ha_init();
void ha_start();
bool ha_uses_wifi();
HaState ha_state();
HaSave ha_save(char *fields); // enabled, SSID, Wi-Fi key, broker, port, user, MQTT key
bool ha_send(int action); // -1 sends a test event; local actions are never queued.
