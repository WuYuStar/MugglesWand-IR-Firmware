#pragma once
#include <stdint.h>

// Stable persisted IDs: -1 follows the function, 0 disables added effects.
constexpr int kGestureEffectMax = 25;
struct GestureEffectChoice { int8_t id; const char *name; const char *group; };
constexpr GestureEffectChoice kGestureEffects[] = {
    {-1,"跟随功能默认灯效","默认"},{0,"不播放附加灯效","默认"},
    {1,"红色流水","流水"},{2,"绿色流水","流水"},{3,"黄色流水","流水"},{4,"紫色流水","流水"},
    {5,"粉色流水","流水"},{6,"蓝色流水","流水"},{7,"青色流水","流水"},{8,"白色流水","流水"},
    {9,"红色闪烁","闪烁"},{10,"绿色闪烁","闪烁"},{11,"黄色闪烁","闪烁"},{12,"紫色闪烁","闪烁"},
    {13,"粉色闪烁","闪烁"},{14,"蓝色闪烁","闪烁"},{15,"青色闪烁","闪烁"},{16,"白色闪烁","闪烁"},
    {17,"红色呼吸","呼吸"},{18,"绿色呼吸","呼吸"},{19,"黄色呼吸","呼吸"},{20,"紫色呼吸","呼吸"},
    {21,"粉色呼吸","呼吸"},{22,"蓝色呼吸","呼吸"},{23,"青色呼吸","呼吸"},{24,"白色呼吸","呼吸"},
    {25,"彩虹流动","其他"}
};
constexpr int kGestureEffectChoiceCount = sizeof(kGestureEffects)/sizeof(kGestureEffects[0]);
inline bool gesture_effect_valid(int id) { return id>=-1 && id<=kGestureEffectMax; }
// The APP task owns dispatch; this never changes the active functional mode.
void gesture_effect_play(int id);
