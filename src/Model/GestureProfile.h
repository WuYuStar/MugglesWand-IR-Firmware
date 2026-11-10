#pragma once
#include <stdint.h>

// This firmware still contains the original model. The trainer generates a
// letters-v1 profile only when installing a genuinely trained letter model.
constexpr const char *kGestureProfileKey = "legacy-v1";
constexpr const char *kGestureProfileTitle = "原版手势模型";
constexpr const char *kGestureLabels[8] = {
    "原手势1", "原手势2", "原手势3 · 向上", "原手势4 · 向下",
    "原手势5 · 鼠标", "原手势6 · 照明", "原手势7 · 音量", "原手势8 · 光绘"};
constexpr int8_t kDefaultGestureActions[8] = {-1, -1, -1, -1, 4, 5, 6, 7};

constexpr int kGestureCount = 8;
struct GestureProfileFallback { const char *key; int count; };
constexpr int kGestureFallbackCount = 0;
constexpr GestureProfileFallback kGestureFallbacks[] = {{"",0}};
