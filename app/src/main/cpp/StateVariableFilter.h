#ifndef ALTOGUITARFX_STATEVARIABLEFILTER_H
#define ALTOGUITARFX_STATEVARIABLEFILTER_H

#include <cmath>

namespace alto {

/**
 * Filter state-variable dengan TPT (topology-preserving transform). Stabil
 * saat frekuensi potong dimodulasi cepat per sampel, sehingga cocok untuk wah
 * yang dikendalikan selubung sinyal. Keluaran band-pass dipakai Auto Wah.
 */
class StateVariableFilter {
public:
    void setSampleRate(double sampleRate) {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
    }

    void reset() {
        ic1_ = 0.0f;
        ic2_ = 0.0f;
    }

    /** Keluaran band-pass pada [cutoffHz] dengan faktor kualitas [q]. */
    float bandPass(float x, float cutoffHz, float q) {
        const float nyquist = static_cast<float>(sampleRate_) * 0.5f;
        float f = cutoffHz;
        if (f < 20.0f) f = 20.0f;
        if (f > nyquist * 0.90f) f = nyquist * 0.90f;
        float qq = q;
        if (qq < 0.5f) qq = 0.5f;

        const float g = std::tan(3.14159265f * f / static_cast<float>(sampleRate_));
        const float k = 1.0f / qq;
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;

        const float v3 = x - ic2_;
        const float v1 = a1 * ic1_ + a2 * v3;
        const float v2 = ic2_ + a2 * ic1_ + a3 * v3;
        ic1_ = 2.0f * v1 - ic1_;
        ic2_ = 2.0f * v2 - ic2_;
        return v1;
    }

private:
    double sampleRate_ = 48000.0;
    float ic1_ = 0.0f;
    float ic2_ = 0.0f;
};

}  // namespace alto

#endif  // ALTOGUITARFX_STATEVARIABLEFILTER_H
