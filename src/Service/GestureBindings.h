#pragma once
#include <Arduino.h>
#include "Model/ModelCatalog.h"

constexpr bool gesture_action_retired(int action) { return action >= 0 && action <= 3; }

struct GestureBindingState {
    int8_t actions[kMaxGestureClasses];
    int8_t effects[kMaxGestureClasses];
    uint32_t revision;
    bool writable;
};

enum class BindingSave { Saved, Invalid, Conflict, StorageError };
void gesture_bindings_init();
GestureBindingState gesture_bindings_state();
int gesture_binding_action(int id);
BindingSave gesture_bindings_save(const int8_t actions[kMaxGestureClasses], uint32_t expectedRevision);
// Strict, bounded WebSocket command: web:bindings <revision> <kGestureCount comma-separated actions>.
bool gesture_bindings_parse(const char *command, int8_t actions[kMaxGestureClasses], uint32_t &revision);

BindingSave gesture_bindings_save(const int8_t actions[kMaxGestureClasses], const int8_t effects[kMaxGestureClasses], uint32_t expectedRevision);
bool gesture_bindings_parse(const char *command, int8_t actions[kMaxGestureClasses], int8_t effects[kMaxGestureClasses], uint32_t &revision);
