#ifndef ALTOGUITARFX_COMPRESSOR_H
#define ALTOGUITARFX_COMPRESSOR_H

#include "Effect.h"

namespace alto {

/**
 * Compressor (BAB 7).
 *
 * Parameter:
 *   0 threshold  : -60..0 dB
 *   1 ratio      : 1..20
 *   2 attack     : 0.5..200 ms
 *   3 release    : 10..1000 ms
 *   4 makeup     : 0..24 dB
 *
 * Detektor RMS dengan soft knee 6 dB. Penguatan dikurangi dalam domain dB lalu
 * dikembalikan ke linier; attack/release dihaluskan per sampel.
 */
class Compressor : public Effect {
public:
    Compressor();

    void prepare(double sampleRate, int32_t channels) override;
    void reset() override;
    void process(float* buffer, int32_t frames, int32_t channels) override;

protected:
    void onParamChanged(int index, float value) override;

private:
    SmoothedParam thresholdDb_;
    SmoothedParam ratio_;
    SmoothedParam attackMs_;
    SmoothedParam releaseMs_;
    SmoothedParam makeupDb_;

    float envelope_ = 0.0f;
    float gainDb_ = 0.0f;
};

}  // namespace alto

#endif  // ALTOGUITARFX_COMPRESSOR_H
