#ifndef ALTOGUITARFX_SATURATION_H
#define ALTOGUITARFX_SATURATION_H

#include <cmath>
#include <cstdint>

namespace alto {

/**
 * Kurva saturasi dan turunan pertamanya (antiderivatif) untuk teknik
 * Antiderivative Anti-Aliasing (ADAA). ADAA mengurangi aliasing pada
 * waveshaping non-linear tanpa oversampling.
 *
 * Rumus ADAA: y[n] = (F(x[n]) - F(x[n-1])) / (x[n] - x[n-1]), dengan
 * F adalah antiderivatif dari fungsi pembentuk f.
 */
namespace saturation {

/** f(x) = tanh(x) — saturasi halus khas tabung/overdrive. */
inline float tanhShape(float x) {
    return std::tanh(x);
}

/** F(x) = ln(cosh(x)) — antiderivatif tanh, stabil untuk |x| besar. */
inline float tanhAntiderivative(float x) {
    const float ax = std::fabs(x);
    // ln(cosh(x)) = |x| + ln(1 + e^{-2|x|}) - ln(2)
    return ax + std::log1p(std::exp(-2.0f * ax)) - 0.6931472f;
}

/** f(x) = hard clip pada ±threshold. */
inline float hardClipShape(float x, float threshold) {
    if (x > threshold) return threshold;
    if (x < -threshold) return -threshold;
    return x;
}

/** Antiderivatif hard clip: integral f. */
inline float hardClipAntiderivative(float x, float threshold) {
    if (x > threshold) {
        return threshold * x - 0.5f * threshold * threshold;
    }
    if (x < -threshold) {
        return -threshold * x - 0.5f * threshold * threshold;
    }
    return 0.5f * x * x;
}

/**
 * f(x) = klip asimetris: ambang positif dan negatif berbeda. Perbedaan ini
 * menghasilkan harmonisa genap yang bertahan setelah DC dihilangkan — ciri
 * khas fuzz.
 */
inline float asymmetricClipShape(float x, float positive, float negative) {
    if (x > positive) return positive;
    if (x < -negative) return -negative;
    return x;
}

inline float asymmetricClipAntiderivative(float x, float positive, float negative) {
    if (x > positive) {
        return positive * x - 0.5f * positive * positive;
    }
    if (x < -negative) {
        return -negative * x - 0.5f * negative * negative;
    }
    return 0.5f * x * x;
}

}  // namespace saturation

/**
 * Saturator ADAA satu kanal. Menyimpan nilai sampel dan antiderivatif
 * sebelumnya. Mendukung hingga 2 kanal (stereo).
 */
class AdaaSaturator {
public:
    /** Mengosongkan keadaan; panggil sebelum audio berjalan. */
    void reset() {
        for (auto& v : prevX_) v = 0.0f;
        for (auto& v : prevF_) v = 0.0f;
    }

    /** Saturasi tanh dengan ADAA. */
    float processTanh(float x, int32_t channel) {
        const float f = saturation::tanhAntiderivative(x);
        const float delta = x - prevX_[channel];
        float y;
        if (std::fabs(delta) < 1e-5f) {
            // Mendekati limit: gunakan nilai fungsi langsung.
            y = saturation::tanhShape(0.5f * (x + prevX_[channel]));
        } else {
            y = (f - prevF_[channel]) / delta;
        }
        prevX_[channel] = x;
        prevF_[channel] = f;
        return y;
    }

    /** Hard clip dengan ADAA. */
    float processHardClip(float x, float threshold, int32_t channel) {
        const float f = saturation::hardClipAntiderivative(x, threshold);
        const float delta = x - prevX_[channel];
        float y;
        if (std::fabs(delta) < 1e-5f) {
            y = saturation::hardClipShape(0.5f * (x + prevX_[channel]), threshold);
        } else {
            y = (f - prevF_[channel]) / delta;
        }
        prevX_[channel] = x;
        prevF_[channel] = f;
        return y;
    }

    /** Klip asimetris dengan ADAA (untuk fuzz). */
    float processAsymmetricClip(float x, float positive, float negative, int32_t channel) {
        const float f = saturation::asymmetricClipAntiderivative(x, positive, negative);
        const float delta = x - prevX_[channel];
        float y;
        if (std::fabs(delta) < 1e-5f) {
            y = saturation::asymmetricClipShape(0.5f * (x + prevX_[channel]), positive, negative);
        } else {
            y = (f - prevF_[channel]) / delta;
        }
        prevX_[channel] = x;
        prevF_[channel] = f;
        return y;
    }

private:
    float prevX_[2]{};
    float prevF_[2]{};
};

/** Blocker DC satu kanal (high-pass sangat rendah) untuk menghilangkan offset. */
class DcBlocker {
public:
    void reset() { x1_ = 0.0f; y1_ = 0.0f; }

    float process(float x) {
        const float y = x - x1_ + 0.995f * y1_;
        x1_ = x;
        y1_ = y;
        return y;
    }

private:
    float x1_ = 0.0f;
    float y1_ = 0.0f;
};

/** Low-pass satu kutub untuk kontrol tone. */
class OnePoleLowPass {
public:
    void reset() { state_ = 0.0f; }

    void setCutoff(float normalized) {
        // normalized 0..1 -> koefisien 0.01..0.9 (makin tinggi makin terang).
        coefficient_ = 0.01f + normalized * 0.89f;
    }

    float process(float x) {
        state_ += (x - state_) * coefficient_;
        return state_;
    }

private:
    float state_ = 0.0f;
    float coefficient_ = 0.5f;
};

}  // namespace alto

#endif  // ALTOGUITARFX_SATURATION_H
