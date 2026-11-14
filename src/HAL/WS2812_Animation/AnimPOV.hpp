#pragma once
#include "AnimationBase.hpp"

typedef struct
{
    const char *name;  // 图案名称 (控制台帮助列表用)
    const CRGB *data;  // 指向 2D const 数组首元素
    uint16_t rows;     // 行数 (= n, 决定播放时长: rows*10ms)
    uint16_t width;    // 图案原始列数，可与灯带长度不同
} POVPatternInfo;

/**
 * @brief POV 视觉暂留图像播放动画（overlay 类型，自动销毁）
 *
 * 逐行将静态图案缓冲区拷入灯带画布（每 10ms tick 推进 1 行 = 100 行/秒），
 * 挥舞灯带时由于视觉暂留，扫过的矩形区域会呈现完整二维图像。
 * 播放时长 = rows * 10ms（20 行 = 200ms，40 行 = 400ms）。
 *
 * 播完仅置完成标志交由引擎自动清理；被 delete（销毁）时通过 _doneFn 通知
 * 上层"动画已结束"（仿 AnimLumos 析构刷新上层状态的范式，仅置标志位，
 * 不涉及渲染命令队列，无竞态）。
 */
class AnimPOV : public AnimationBase
{
private:
    const POVPatternInfo *_pattern; // 指向 flash 中的静态图案
    uint16_t _rowIndex;             // 当前行号（每 tick 递增 1）
    bool _reverse;                  // true = 从最后一行向第一行播放（适配反向挥舞）
    uint8_t _brightness;            // 全局亮度缩放 0~255，默认 255（图案已预缩放）
    void (*_doneFn)(void);          // 销毁回调：动画被 delete 时调用（通知上层播放已结束）

public:
    AnimPOV(const POVPatternInfo *pattern, bool reverse = false,
            uint8_t brightness = 255, void (*doneFn)(void) = nullptr)
        : _pattern(pattern), _rowIndex(0),
          _reverse(reverse), _brightness(brightness), _doneFn(doneFn) {}

    // 析构：通知上层动画已销毁（此刻即"播放完成销毁后"的信号）
    ~AnimPOV() override
    {
        if (_doneFn != nullptr)
            _doneFn();
    }

    void init() override
    {
        AnimationBase::init();
        _rowIndex = 0;
    }

    void update(CRGB *leds, uint16_t numLeds) override
    {
        // 防御：图案无效或已播完（引擎会在 finished 后自动 delete 本对象）
        if (_pattern == nullptr || _rowIndex >= _pattern->rows)
        {
            _isFinished = true;
            return;
        }

        // 计算源行（reverse = 从最后一行倒播，适配反向挥舞时不倒立）
        uint16_t srcRow = _reverse ? (_pattern->rows - 1 - _rowIndex) : _rowIndex;
        const CRGB *src = &_pattern->data[srcRow * _pattern->width];
        if (_pattern->width == 0) { _isFinished = true; return; }
        // 把图案的原始列均匀映射到当前灯带长度，避免 35～45 颗时裁掉末端或留下黑尾。
        for (uint16_t i = 0; i < numLeds; i++)
        {
            const uint16_t source = numLeds > 1
                ? (i * (_pattern->width - 1) + (numLeds - 1) / 2) / (numLeds - 1) : 0;
            CRGB c = src[source];
            if (_brightness < 255) c.nscale8(_brightness);
            leds[i] = c;
        }

        _rowIndex++; // 每 tick 推进 1 行（频率 = 1000 / 帧间隔 ms）

        // 播完最后一行：置完成标志，引擎下一 tick 自动 delete（析构中通知上层 _doneFn）
        if (_rowIndex >= _pattern->rows)
        {
            _isFinished = true;
        }
    }
};
