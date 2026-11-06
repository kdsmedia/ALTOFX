#ifndef ALTOGUITARFX_CHORUS_H
#define ALTOGUITARFX_CHORUS_H

#include "Effect.h"
#include "Modulation.h"

namespace alto {

/**
 * Chorus (BAB 10): modulasi waktu tunda (delay ~15-35 ms) oleh LFO, dicampur
 * dengan sinyal kering untuk kesan lebar. Parameter: Rate, Depth, Mix.
 */
class Chorus : public Effect {
public:
    static constexpr float kBaseDelayMs = 22.0f;
    static constexpr float kMaxModulationMs = 12.0f;
    static constexpr int32_t kMaxChannels = 2;

    Chorus();

    void prepare(double sampleRate, int32_t channels) override;
    void reset() override;
    void process(float* buffer, int32_t frames, int32_t channels) override;

protected:
    void onParamChanged(int index, float value) override;

private:
    SmoothedParam rate_;
    SmoothedParam depth_;
    SmoothedParam mix_;
    Lfo lfo_;
    DelayLine delay_;
};

}  // namespace alto

#endif  // ALTOGUITARFX_CHORUS_H
