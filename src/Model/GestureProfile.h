#pragma once
#include <stdint.h>

constexpr const char *kGestureProfileKey = "letters-v1";
constexpr const char *kGestureProfileTitle = "字母手势模型";
constexpr const char *kGestureLabels[8] = {"a", "e", "n", "o", "p", "s", "8", "∞"};
constexpr int8_t kDefaultGestureActions[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
constexpr int kGestureCount = 8;
struct GestureProfileFallback { const char *key; int count; };
constexpr int kGestureFallbackCount = 0;
constexpr GestureProfileFallback kGestureFallbacks[] = {{"", 0}};
