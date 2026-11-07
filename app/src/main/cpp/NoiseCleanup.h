#ifndef ALTOGUITARFX_NOISECLEANUP_H
#define ALTOGUITARFX_NOISECLEANUP_H

#include <cmath>
#include <cstdint>

namespace alto {

/**
 * Pembersih bising ringan per efek (tombol "Noise" pada tiap efek).
 *
 * Berupa gerbang lembut (downward expander) yang menurunkan derau latar
 * (hiss) ketika sinyal berada di bawah ambang derau. Dipakai SEBELUM tiap
 * efek sehingga derau tidak ikut dikuatkan oleh efek berikutnya (khususnya
 * efek saturasi).
 *
 * Sengaja TIDAK memakai filter high-pass/DC di sini: penyaringan dengung dan
 * frekuensi rendah sudah dilakukan sekali di jalur input (HumFilter), dan
 * menumpuknya per efek akan memotong nada bas gitar. Yang ditumpuk di sini
 * hanya peredaman derau di wilayah sunyi — aman karena tidak memengaruhi nada
 * yang sedang berbunyi (di atas ambang, gerbang terbuka penuh).
 *
 * Histeresis dan waktu tahan mencegah gerbang memotong nada yang meluruh.
 * Tidak ada alokasi memori di jalur audio.
 */
class NoiseCleanup {
public:
    static constexpr int32_t kMaxChannels = 2;

    void prepare(double sampleRate, int32_t channels) {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        channels_ = channels > 0 ? channels : 1;
        if (channels_ > kMaxChannels) channels_ = kMaxChannels;
        envelopeCoeff_ = timeToCoefficient(kEnvelopeMs, sampleRate_);
        releaseCoeff_ = timeToCoefficient(kReleaseMs, sampleRate_);
        holdSamples_ = static_cast<int32_t>(kHoldMs * 0.001f * static_cast<float>(sampleRate_));
        reset();
    }

    void reset() {
        envelope_ = 0.0f;
        gain_ = 1.0f;
        holdCounter_ = 0;
        gateOpen_ = false;
    }

    void process(float* buffer, int32_t frames, int32_t channels) {
        const int32_t ch = channels < channels_ ? channels : channels_;
        const float openThreshold = kThreshold;
        const float closeThreshold = kThreshold * 0.707f;

        for (int32_t frame = 0; frame < frames; ++frame) {
            float peak = 0.0f;
            for (int32_t c = 0; c < ch; ++c) {
                const float sample = buffer[frame * channels + c];
                const float magnitude = sample < 0.0f ? -sample : sample;
                if (magnitude > peak) peak = magnitude;
            }
            envelope_ += (peak - envelope_) * envelopeCoeff_;

            if (envelope_ >= openThreshold) {
                gateOpen_ = true;
                holdCounter_ = holdSamples_;
            } else if (gateOpen_ && envelope_ <= closeThreshold) {
                if (holdCounter_ > 0) {
                    --holdCounter_;
                } else {
                    gateOpen_ = false;
                }
            }

            const float targetGain = gateOpen_ ? 1.0f : kFloorGain;
            gain_ += (targetGain - gain_) * releaseCoeff_;
            for (int32_t c = 0; c < ch; ++c) {
                buffer[frame * channels + c] *= gain_;
            }
        }
    }

private:
    static float timeToCoefficient(float ms, double sampleRate) {
        if (ms <= 0.0f) return 1.0f;
        return 1.0f - std::exp(-1.0f / (ms * 0.001f * static_cast<float>(sampleRate)));
    }

    static constexpr float kEnvelopeMs = 2.0f;
    static constexpr float kReleaseMs = 80.0f;
    static constexpr float kHoldMs = 60.0f;
    static constexpr float kThreshold = 0.0018f;  // ~ -55 dBFS
    static constexpr float kFloorGain = 0.15f;    // sisa lembut, bukan bisu penuh

    double sampleRate_ = 48000.0;
    int32_t channels_ = 1;
    float envelopeCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
    int32_t holdSamples_ = 0;
    int32_t holdCounter_ = 0;
    float envelope_ = 0.0f;
    float gain_ = 1.0f;
    bool gateOpen_ = false;
};

}  // namespace alto

#endif  // ALTOGUITARFX_NOISECLEANUP_H
