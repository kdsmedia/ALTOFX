#ifndef ALTOGUITARFX_FLANGER_H
#define ALTOGUITARFX_FLANGER_H

#include "Effect.h"
#include "Modulation.h"

namespace alto {

/**
 * Flanger (BAB 10): delay sangat pendek (0.5-6 ms) dengan umpan balik,
 * menghasilkan comb filter yang bergerak. Parameter: Rate, Depth, Feedback,
 * Mix.
 */
class Flanger : public Effect {
public:
    static constexpr float kBaseDelayMs = 1.0f;
    static constexpr float kMaxModulationMs = 5.0f;
    static constexpr int32_t kMaxChannels = 2;

    Flanger();

    void prepare(double sampleRate, int32_t channels) override;
    void reset() override;
    void process(float* buffer, int32_t frames, int32_t channels) override;

protected:
    void onParamChanged(int index, float value) override;

private:
    SmoothedParam rate_;
    SmoothedParam depth_;
    SmoothedParam feedback_;
    SmoothedParam mix_;
    Lfo lfo_;
    DelayLine delay_;
    float feedbackSample_[kMaxChannels]{};
};

}  // namespace alto

#endif  // ALTOGUITARFX_FLANGER_H
