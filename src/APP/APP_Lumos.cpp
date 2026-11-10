#include "APP_Lumos.h"
#include "HAL/HAL.h"
#include "HAL/WS2812_Animation/AnimLumos.hpp"
#include <atomic>

// Low bit: on. Other bits identify the current animation, so an old queued
// animation's destructor cannot turn a newer lamp off.
static std::atomic<uint32_t> lampState{0};
void APP_Lumos_trigger(uint32_t color){
    if(lampState.load()&1)APP_Lumos_off();else APP_Lumos_on(color);
}
void APP_Lumos_on(uint32_t color){
    uint32_t token=(lampState.fetch_add(2)+2)|1u;
    lampState.store(token);
    HAL::ws2812_set_overlay(new AnimLumos(CRGB(color),&lampState,token));
}
void APP_Lumos_off(){
    lampState.store((lampState.load()+2)&~1u);
    HAL::enable_AnimStatus();
}
