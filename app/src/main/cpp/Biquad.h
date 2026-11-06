#ifndef ALTOGUITARFX_BIQUAD_H
#define ALTOGUITARFX_BIQUAD_H

#include <cmath>

namespace alto {

/**
 * Filter biquad (direct form I) dengan koefisien dari buku resep RBJ
 * (Audio EQ Cookbook). Dipakai untuk equalizer dan efek lain.
 */
class Biquad {
public:
    void reset() { x1_ = x2_ = y1_ = y2_ = 0.0f; }

    void setLowShelf(float sampleRate, float frequency, float q, float gainDb) {
        const float a = std::pow(10.0f, gainDb / 40.0f);
        const float w = omega(sampleRate, frequency);
        const float cosW = std::cos(w);
        const float alpha = std::sin(w) / (2.0f * q);
        const float twoSqrtAAlpha = 2.0f * std::sqrt(a) * alpha;

        const float b0 = a * ((a + 1.0f) - (a - 1.0f) * cosW + twoSqrtAAlpha);
        const float b1 = 2.0f * a * ((a - 1.0f) - (a + 1.0f) * cosW);
        const float b2 = a * ((a + 1.0f) - (a - 1.0f) * cosW - twoSqrtAAlpha);
        const float a0 = (a + 1.0f) + (a - 1.0f) * cosW + twoSqrtAAlpha;
        const float a1 = -2.0f * ((a - 1.0f) + (a + 1.0f) * cosW);
        const float a2 = (a + 1.0f) + (a - 1.0f) * cosW - twoSqrtAAlpha;
        normalize(b0, b1, b2, a0, a1, a2);
    }

    void setHighShelf(float sampleRate, float frequency, float q, float gainDb) {
        const float a = std::pow(10.0f, gainDb / 40.0f);
        const float w = omega(sampleRate, frequency);
        const float cosW = std::cos(w);
        const float alpha = std::sin(w) / (2.0f * q);
        const float twoSqrtAAlpha = 2.0f * std::sqrt(a) * alpha;

        const float b0 = a * ((a + 1.0f) + (a - 1.0f) * cosW + twoSqrtAAlpha);
        const float b1 = -2.0f * a * ((a - 1.0f) + (a + 1.0f) * cosW);
        const float b2 = a * ((a + 1.0f) + (a - 1.0f) * cosW - twoSqrtAAlpha);
        const float a0 = (a + 1.0f) - (a - 1.0f) * cosW + twoSqrtAAlpha;
        const float a1 = 2.0f * ((a - 1.0f) - (a + 1.0f) * cosW);
        const float a2 = (a + 1.0f) - (a - 1.0f) * cosW - twoSqrtAAlpha;
        normalize(b0, b1, b2, a0, a1, a2);
    }

    void setPeaking(float sampleRate, float frequency, float q, float gainDb) {
        const float a = std::pow(10.0f, gainDb / 40.0f);
        const float w = omega(sampleRate, frequency);
        const float cosW = std::cos(w);
        const float alpha = std::sin(w) / (2.0f * q);

        const float b0 = 1.0f + alpha * a;
        const float b1 = -2.0f * cosW;
        const float b2 = 1.0f - alpha * a;
        const float a0 = 1.0f + alpha / a;
        const float a1 = -2.0f * cosW;
        const float a2 = 1.0f - alpha / a;
        normalize(b0, b1, b2, a0, a1, a2);
    }

    float process(float x) {
        const float y = b0_ * x + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
        x2_ = x1_;
        x1_ = x;
        y2_ = y1_;
        y1_ = y;
        return y;
    }

private:
    static float omega(float sampleRate, float frequency) {
        const float nyquist = sampleRate * 0.5f;
        float f = frequency;
        if (f < 1.0f) f = 1.0f;
        if (f > nyquist * 0.99f) f = nyquist * 0.99f;
        return 2.0f * static_cast<float>(M_PI) * f / sampleRate;
    }

    void normalize(float b0, float b1, float b2, float a0, float a1, float a2) {
        b0_ = b0 / a0;
        b1_ = b1 / a0;
        b2_ = b2 / a0;
        a1_ = a1 / a0;
        a2_ = a2 / a0;
    }

    float b0_ = 1.0f, b1_ = 0.0f, b2_ = 0.0f, a1_ = 0.0f, a2_ = 0.0f;
    float x1_ = 0.0f, x2_ = 0.0f, y1_ = 0.0f, y2_ = 0.0f;
};

}  // namespace alto

#endif  // ALTOGUITARFX_BIQUAD_H
