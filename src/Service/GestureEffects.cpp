#include "GestureEffects.h"
#include "HAL/HAL.h"
#include "HAL/WS2812_Animation/AnimGestureFx.hpp"
#include "HAL/WS2812_Animation/AnimFlow.hpp"

void gesture_effect_play(int id) {
    if (id==0) { HAL::ws2812_start_fx(nullptr);return; }
    if (id<1 || id>kGestureEffectMax) return;
    constexpr uint32_t colors[]={0xFF2020,0x20FF50,0xFFD000,0xA84CFF,0xFF60B5,0x3080FF,0x28E8D8,0xFFFFFF};
    if (id==25) HAL::ws2812_start_fx(new AnimGestureFx(3,CRGB::White));
    else if (id<=8) HAL::ws2812_start_fx(new AnimFlow(CRGB(colors[id-1])));
    else HAL::ws2812_start_fx(new AnimGestureFx((id-1)/8,CRGB(colors[(id-1)%8])));
}
