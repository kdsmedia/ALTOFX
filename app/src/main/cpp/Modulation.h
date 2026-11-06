#ifndef ALTOGUITARFX_MODULATION_H
#define ALTOGUITARFX_MODULATION_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace alto {

/**
 * Osilator frekuensi rendah (LFO) sinus, keluaran -1..1.
 */
class Lfo {
public:
    void setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }

    void setRate(float hz) { rate_ = hz; }

    void reset() { phase_ = 0.0f; }

    float next() {
        phase_ += rate_ / static_cast<float>(sampleRate_);
        if (phase_ >= 1.0f) {
            phase_ -= 1.0f;
        }
        return std::sin(2.0f * static_cast<float>(M_PI) * phase_);
    }

private:
    double sampleRate_ = 48000.0;
    float rate_ = 1.0f;
    float phase_ = 0.0f;
};

/**
 * Delay line dengan pembacaan fraksional (interpolasi linear). Dipakai untuk
 * chorus dan flanger. Memori dialokasikan di [prepare], bukan di jalur audio.
 */
class DelayLine {
public:
    void prepare(double sampleRate, float maxDelayMs, int32_t channels) {
        channels_ = channels;
        size_ = static_cast<int32_t>(maxDelayMs * 0.001f * static_cast<float>(sampleRate)) + 4;
        buffers_.assign(channels, std::vector<float>(size_, 0.0f));
        writeIndex_ = 0;
    }

    void reset() {
        for (auto& buffer : buffers_) {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
        }
        writeIndex_ = 0;
    }

    void write(float sample, int32_t channel) { buffers_[channel][writeIndex_] = sample; }

    float read(float delaySamples, int32_t channel) const {
        float d = delaySamples;
        if (d < 1.0f) {
            d = 1.0f;
        }
        if (d > static_cast<float>(size_ - 2)) {
            d = static_cast<float>(size_ - 2);
        }
        float position = static_cast<float>(writeIndex_) - d;
        while (position < 0.0f) {
            position += static_cast<float>(size_);
        }
        const int32_t i0 = static_cast<int32_t>(position);
        const float fraction = position - static_cast<float>(i0);
        const int32_t i1 = (i0 + 1) % size_;
        return buffers_[channel][i0] * (1.0f - fraction) + buffers_[channel][i1] * fraction;
    }

    void advance() { writeIndex_ = (writeIndex_ + 1) % size_; }

private:
    std::vector<std::vector<float>> buffers_;
    int32_t size_ = 0;
    int32_t writeIndex_ = 0;
    int32_t channels_ = 1;
};

/**
 * Satu tingkat filter all-pass orde satu untuk phaser. Koefisien a menentukan
 * frekuensi putar dan dimodulasi oleh LFO.
 */
class AllPassStage {
public:
    void reset() { state_ = 0.0f; }

    float process(float x, float a) {
        const float y = -a * x + state_;
        state_ = x + a * y;
        return y;
    }

private:
    float state_ = 0.0f;
};

/** Koefisien all-pass dari frekuensi putar (Hz). */
inline float allPassCoefficient(float frequency, double sampleRate) {
    const float nyquist = static_cast<float>(sampleRate) * 0.5f;
    float f = frequency;
    if (f < 10.0f) f = 10.0f;
    if (f > nyquist * 0.95f) f = nyquist * 0.95f;
    const float t = std::tan(static_cast<float>(M_PI) * f / static_cast<float>(sampleRate));
    return (t - 1.0f) / (t + 1.0f);
}

}  // namespace alto

#endif  // ALTOGUITARFX_MODULATION_H
