#ifndef ALTOGUITARFX_TREMOLO_H
#define ALTOGUITARFX_TREMOLO_H

#include "Effect.h"
#include "Modulation.h"

namespace alto {

/**
 * Tremolo (BAB 10): modulasi amplitudo periodik oleh LFO. Parameter: Rate,
 * Depth.
 */
class Tremolo : public Effect {
public:
    Tremolo();

    void prepare(double sampleRate, int32_t channels) override;
    void reset() override;
    void process(float* buffer, int32_t frames, int32_t channels) override;

protected:
    void onParamChanged(int index, float value) override;

private:
    SmoothedParam rate_;
    SmoothedParam depth_;
    Lfo lfo_;
};

}  // namespace alto

#endif  // ALTOGUITARFX_TREMOLO_H
