#pragma once
#include "AnimationBase.hpp"

// Bounded blink/breathe/rainbow presets. Flow uses the original AnimFlow.
class AnimGestureFx : public AnimationBase {
    uint8_t style;
    CRGB color;
public:
    AnimGestureFx(uint8_t kind, CRGB value) : style(kind),color(value) {}
    void update(CRGB *leds,uint16_t count) override {
        const uint32_t elapsed=millis()-_startTime;
        const uint32_t duration=style==1?900:1800;
        if (!count || elapsed>=duration) { _isFinished=true;return; }
        if (style==3) {
            for (uint16_t i=0;i<count;++i) leds[i]+=CHSV(static_cast<uint8_t>(elapsed/8+i*255/count),255,42);
        } else {
            uint8_t brightness=0;
            const uint8_t peak=(color.r==color.g && color.g==color.b)?24:40;
            if (style==1) brightness=(elapsed%300)<140?peak:0;
            else brightness=elapsed<duration/2?elapsed*2*peak/duration:(duration-elapsed)*2*peak/duration;
            CRGB pixel=color;pixel.nscale8(brightness);
            for (uint16_t i=0;i<count;++i) leds[i]+=pixel;
        }
    }
};
