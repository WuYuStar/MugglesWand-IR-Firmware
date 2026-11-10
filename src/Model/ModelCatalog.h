#pragma once
#include <stdint.h>
constexpr int kMaxGestureClasses=16;
int gesture_model_count();
const char *gesture_model_label(int index);
const char *gesture_model_key();
const char *gesture_model_title();
int gesture_model_default_action(int index);
int gesture_model_fallback_count();
const char *gesture_model_fallback_key(int index);
int gesture_model_fallback_size(int index);
