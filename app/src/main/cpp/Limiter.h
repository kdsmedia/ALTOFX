#ifndef ALTOGUITARFX_LIMITER_H
#define ALTOGUITARFX_LIMITER_H

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace alto {

/**
 * Limiter puncak dengan lookahead pada jalur keluaran master (BAB 15).
 *
 * Menggantikan pemotongan keras (hard clip) lama yang, pada level output
 * master tinggi (+12 dB bawaan), memangkas puncak secara mendadak sehingga
 * menghasilkan harmonisa kasar — terdengar sebagai "pecah"/crackle. Limiter
 * ini menahan puncak dengan menurunkan gain secara halus SEBELUM puncak tiba
 * (lookahead), lalu memulihkannya perlahan, sehingga keluaran tidak pernah
 * melewati batas (ceiling) tanpa mengubah sinyal yang di bawah batas.
 *
 * Deteksi puncak memakai maksimum antar-kanal (kanal terhubung) agar citra
 * stereo tidak bergeser. Tidak ada alokasi memori di jalur audio.
 */
class Limiter {
public:
    static constexpr int kMaxChannels = 2;
    // Cukup untuk lookahead 1,5 ms pada sample rate hingga ~170 kHz.
    static constexpr int kMaxLookaheadSamples = 256;

    /** Konversi desibel ke faktor linier. */
    static float dbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }

    /** Menyiapkan limiter untuk [sampleRate] dan [channels]. */
    void prepare(double sampleRate, int32_t channels, float ceilingDb = 0.0f) {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        channels_ = channels < 1 ? 1 : (channels > kMaxChannels ? kMaxChannels : channels);
        ceiling_ = dbToLinear(ceilingDb);
        lookahead_ = static_cast<int>(kLookaheadMs * 0.001 * sampleRate_);
        if (lookahead_ > kMaxLookaheadSamples) lookahead_ = kMaxLookaheadSamples;
        if (lookahead_ < 1) lookahead_ = 1;
        // Waktu serang disetel ke panjang lookahead: gain sudah nyaris turun
        // penuh tepat saat puncak selesai melewati tunda.
        attackCoeff_ = 1.0f - std::exp(-4.0f / static_cast<float>(lookahead_));
        releaseCoeff_ = coefficient(kReleaseMs);
        detectorReleaseCoeff_ = coefficient(kDetectorReleaseMs);
        reset();
    }

    /** Batas puncak keluaran (desibel). 0 dBFS berarti tidak melewati skala penuh. */
    void setCeilingDb(float db) { ceiling_ = dbToLinear(db); }

    void reset() {
        for (int32_t c = 0; c < kMaxChannels; ++c) {
            for (int32_t i = 0; i < kMaxLookaheadSamples; ++i) {
                delay_[c][i] = 0.0f;
            }
        }
        writeIndex_ = 0;
        envelope_ = 0.0f;
        gain_ = 1.0f;
    }

    /** true bila limiter sedang menurunkan gain (indikasi puncak tinggi). */
    bool limiting() const { return gain_ < 0.999f; }

    /** Menahan puncak buffer keluaran di tempat. Dipanggil dari jalur audio. */
    void process(float* buffer, int32_t frames, int32_t channels) {
        const int32_t ch = channels < channels_ ? channels : channels_;
        for (int32_t frame = 0; frame < frames; ++frame) {
            float* samples = buffer + static_cast<size_t>(frame) * channels;

            // Puncak masukan blok ini (kanal terhubung).
            float peak = 0.0f;
            for (int32_t c = 0; c < ch; ++c) {
                const float magnitude = std::fabs(samples[c]);
                if (magnitude > peak) peak = magnitude;
            }

            // Selubung deteksi: serangan seketika, peluruhan lambat.
            if (peak > envelope_) {
                envelope_ = peak;
            } else {
                envelope_ += detectorReleaseCoeff_ * (peak - envelope_);
            }

            // Gain yang diperlukan agar selubung tidak melewati batas.
            const float targetGain = (envelope_ > ceiling_) ? (ceiling_ / envelope_) : 1.0f;

            // Perataan gain: turun cepat (attack) saat menahan, naik lambat
            // (release) agar pemulihan tidak terdengar "terpompa".
            const float coeff = (targetGain < gain_) ? attackCoeff_ : releaseCoeff_;
            gain_ += coeff * (targetGain - gain_);

            // Sampel keluaran diambil dari tunda lookahead, lalu masukan
            // sekarang disimpan untuk keluar [lookahead_] sampel kemudian.
            for (int32_t c = 0; c < ch; ++c) {
                const float delayed = delay_[c][writeIndex_];
                delay_[c][writeIndex_] = samples[c];
                float out = delayed * gain_;
                // Pengaman akhir: jangan pernah melewati batas.
                if (out > ceiling_) out = ceiling_;
                else if (out < -ceiling_) out = -ceiling_;
                samples[c] = out;
            }
            if (++writeIndex_ >= lookahead_) writeIndex_ = 0;
        }
    }

private:
    float coefficient(float ms) const {
        if (ms <= 0.0f) return 1.0f;
        return 1.0f - std::exp(-1.0f / (static_cast<float>(sampleRate_) * ms * 0.001f));
    }

    static constexpr float kLookaheadMs = 1.5f;
    static constexpr float kReleaseMs = 120.0f;
    static constexpr float kDetectorReleaseMs = 150.0f;

    double sampleRate_ = 48000.0;
    int32_t channels_ = 2;
    int32_t lookahead_ = 1;
    float ceiling_ = 1.0f;
    float attackCoeff_ = 1.0f;
    float releaseCoeff_ = 0.0f;
    float detectorReleaseCoeff_ = 0.0f;
    float delay_[kMaxChannels][kMaxLookaheadSamples]{};
    int32_t writeIndex_ = 0;
    float envelope_ = 0.0f;
    float gain_ = 1.0f;
};

}  // namespace alto

#endif  // ALTOGUITARFX_LIMITER_H
