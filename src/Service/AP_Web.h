#pragma once

#include <Arduino.h>

class AsyncWebServerRequest;
bool ap_web_local(AsyncWebServerRequest *request);

// AP Web 服务控制接口
void ap_web_start(void);
void ap_web_stop(void);

// 向所有 WebSocket 客户端广播日志数据
void ap_web_print(const char *buffer, size_t length);

// Explicit result/acceptance for beginner practice feedback (not rounded log parsing).
void ap_web_gesture_result(int id, float confidence, bool accepted, int action, int effect);

bool ap_web_has_clients();
