#ifndef ALTOGUITARFX_NOISEGATE_H
#define ALTOGUITARFX_NOISEGATE_H

#include "Effect.h"

namespace alto {

/**
 * Noise Gate (BAB 7).
 *
 * Parameter (indeks -> rentang):
 *   0 threshold  : -80..0 dB (level saat gate mulai menutup)
 *   1 release    : 5..500 ms (waktu menutup setelah sinyal hilang)
 *   2 attack     : 1..50 ms (waktu membuka saat sinyal melewati ambang)
 *
 * Detektor memakai selubung (envelope) dengan histeresis 3 dB agar gate tidak
 * berkedip (chatter) pada sinyal di sekitar ambang.
 */
class NoiseGate : public Effect {
public:
    NoiseGate();

    void prepare(double sampleRate, int32_t channels) override;
    void reset() override;
    void process(float* buffer, int32_t frames, int32_t channels) override;

protected:
    void onParamChanged(int index, float value) override;

private:
    SmoothedParam thresholdDb_;
    SmoothedParam releaseMs_;
    SmoothedParam attackMs_;

    float envelope_ = 0.0f;
    float gain_ = 0.0f;
    float envelopeCoefficient_ = 0.0f;
    float attackCoefficient_ = 0.0f;
    float releaseCoefficient_ = 0.0f;
};

}  // namespace alto

#endif  // ALTOGUITARFX_NOISEGATE_H
