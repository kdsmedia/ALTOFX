#ifndef ALTOGUITARFX_PHASER_H
#define ALTOGUITARFX_PHASER_H

#include "Effect.h"
#include "Modulation.h"

namespace alto {

/**
 * Phaser (BAB 10): deret filter all-pass orde satu yang frekuensi putarnya
 * dimodulasi LFO, dicampur dengan sinyal kering sehingga terbentuk takik
 * (notch) yang berputar. Parameter: Rate, Depth, Feedback, Mix.
 */
class Phaser : public Effect {
public:
    static constexpr int32_t kStages = 4;
    static constexpr int32_t kMaxChannels = 2;
    static constexpr float kMinFrequency = 200.0f;
    static constexpr float kMaxFrequency = 2000.0f;

    Phaser();

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
    AllPassStage stages_[kMaxChannels][kStages];
    float feedbackSample_[kMaxChannels]{};
};

}  // namespace alto

#endif  // ALTOGUITARFX_PHASER_H
