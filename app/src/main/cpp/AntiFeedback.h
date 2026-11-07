#ifndef ALTOGUITARFX_ANTIFEEDBACK_H
#define ALTOGUITARFX_ANTIFEEDBACK_H

#include <cmath>
#include <cstdint>

#include "Biquad.h"
#include "StateVariableFilter.h"

namespace alto {

/**
 * Penekan feedback otomatis (anti-howling) pada jalur input.
 *
 * Feedback akustik (mikrofon/gitar menangkap suara speaker lalu menguatkannya
 * berulang) muncul sebagai nada sempit yang KUAT, TETAP, dan bertambah —
 * berbeda dari petikan gitar yang meluruh. Blok ini memantau pita frekuensi
 * dengan bank band-pass, menemukan pita yang dominan secara terus-menerus,
 * lalu memasang notch sempit yang mendalam perlahan dan melepasnya sangat
 * lambat. Dengan cara ini suara tetap jernih tanpa mengorbankan nada gitar
 * yang sedang meluruh.
 *
 * Deteksi dijalankan pada satu kanal (gitar bersifat mono), notch diterapkan
 * ke seluruh kanal. Tidak ada alokasi memori di jalur audio.
 */
class AntiFeedback {
public:
    static constexpr int kMaxChannels = 2;
    static constexpr int kBands = 24;
    static constexpr int kMaxNotches = 3;

    void prepare(double sampleRate, int32_t channels) {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        channels_ = channels > 0 ? channels : 1;
        if (channels_ > kMaxChannels) channels_ = kMaxChannels;

        // Pita logaritmik dari 100 Hz sampai 5 kHz (rentang feedback khas).
        const float fLow = 100.0f;
        const float fHigh = 5000.0f;
        for (int b = 0; b < kBands; ++b) {
            const float t = static_cast<float>(b) / static_cast<float>(kBands - 1);
            center_[b] = fLow * std::pow(fHigh / fLow, t);
            detector_[b].setSampleRate(sampleRate_);
        }
        // Band-pass detektor cukup sempit agar satu nada menonjol jelas.
        detectorQ_ = 3.0f;
        envelopeCoeff_ = timeToCoefficient(120.0f, sampleRate_);
        confirmFrames_ = static_cast<int32_t>(0.4 * sampleRate_);
        attackFrames_ = static_cast<int32_t>(0.5 * sampleRate_);
        releaseFrames_ = static_cast<int32_t>(8.0 * sampleRate_);
        reset();
    }

    void reset() {
        for (int b = 0; b < kBands; ++b) {
            detector_[b].reset();
            bandEnv_[b] = 0.0f;
        }
        for (int n = 0; n < kMaxNotches; ++n) {
            notchDepth_[n] = 0.0f;
            notchFrequency_[n] = 0.0f;
            notchIdleFrames_[n] = 0;
            for (int c = 0; c < kMaxChannels; ++c) {
                notch_[n][c].reset();
            }
        }
        candidateFrames_ = 0;
        candidateBand_ = -1;
        previousCandidateEnv_ = 0.0f;
    }

    /** Memproses blok audio di tempat (notch diterapkan ke seluruh kanal). */
    void process(float* buffer, int32_t frames, int32_t channels) {
        const int32_t ch = channels < channels_ ? channels : channels_;

        for (int32_t frame = 0; frame < frames; ++frame) {
            const float sample = buffer[frame * channels];

            // Bank detektor pada kanal pertama (sinyal gitar mono).
            for (int b = 0; b < kBands; ++b) {
                const float band = detector_[b].bandPass(sample, center_[b], detectorQ_);
                const float bandMagnitude = band < 0.0f ? -band : band;
                bandEnv_[b] += (bandMagnitude - bandEnv_[b]) * envelopeCoeff_;
            }

            // Terapkan notch aktif ke semua kanal.
            for (int32_t c = 0; c < ch; ++c) {
                float value = buffer[frame * channels + c];
                for (int n = 0; n < kMaxNotches; ++n) {
                    if (notchDepth_[n] > 0.0f) {
                        const float filtered = notch_[n][c].process(value);
                        value += notchDepth_[n] * (filtered - value);
                    }
                }
                buffer[frame * channels + c] = value;
            }
        }

        updateNotches(frames);
    }

private:
    static float timeToCoefficient(float ms, double sampleRate) {
        if (ms <= 0.0f) return 1.0f;
        return 1.0f - std::exp(-1.0f / (ms * 0.001f * static_cast<float>(sampleRate)));
    }

    /** Mencari pita dominan dan memperbarui notch. Dipanggil sekali per blok. */
    void updateNotches(int32_t frames) {
        // Pita terkuat dan pita median (untuk mengukur "menonjol").
        int strongest = 0;
        for (int b = 1; b < kBands; ++b) {
            if (bandEnv_[b] > bandEnv_[strongest]) strongest = b;
        }
        float sorted[kBands];
        for (int b = 0; b < kBands; ++b) sorted[b] = bandEnv_[b];
        for (int i = 1; i < kBands; ++i) {
            const float key = sorted[i];
            int j = i - 1;
            while (j >= 0 && sorted[j] > key) {
                sorted[j + 1] = sorted[j];
                --j;
            }
            sorted[j + 1] = key;
        }
        const float median = sorted[kBands / 2];
        const float strongestEnv = bandEnv_[strongest];

        // Estimasi frekuensi lebih halus dengan interpolasi parabola pada
        // energi log antar tiga pita di sekitar puncak. Bank pita saja terlalu
        // kasar; interpolasi menempatkan notch tepat pada frekuensi feedback.
        const float frequency = refineFrequency(strongest);

        // Kandidat feedback: level tinggi (feedback selalu nyaring), sangat
        // menonjol di atas pita lain, dan tidak meluruh. Nada gitar biasa punya
        // banyak harmonisa sehingga energi tersebar dan tidak menonjol sedemikian.
        const bool loud = strongestEnv > kMinLevel;
        const bool prominent = strongestEnv > kProminenceRatio * (median + 1e-6f);
        const bool sustained = strongestEnv >= previousCandidateEnv_ * 0.98f;
        const bool sameBand = (strongest == candidateBand_);
        previousCandidateEnv_ = strongestEnv;

        if (loud && prominent && sustained) {
            candidateFrames_ = sameBand ? candidateFrames_ + frames : 0;
            candidateBand_ = strongest;
        } else {
            candidateFrames_ = 0;
            candidateBand_ = -1;
        }

        const bool confirmed = candidateFrames_ >= confirmFrames_;

        // Perbarui hitungan "menganggur" tiap notch: notch yang tidak lagi
        // menjadi kandidat (feedback mereda) dilepas perlahan; notch yang masih
        // cocok dipertahankan (penting: notch meredam nada sehingga deteksi
        // menurun, jadi notch tidak boleh langsung dilepas sendiri).
        const int activeSlot = (loud && prominent) ? findNotchNear(frequency) : -1;
        for (int n = 0; n < kMaxNotches; ++n) {
            if (notchDepth_[n] <= 0.0f) continue;
            if (n == activeSlot) {
                notchIdleFrames_[n] = 0;
            } else {
                notchIdleFrames_[n] += frames;
            }
        }

        if (confirmed) {
            int slot = findNotchNear(frequency);
            if (slot < 0) slot = findFreeNotch();
            if (slot >= 0) {
                notchIdleFrames_[slot] = 0;
                // Perbarui koefisien bila frekuensi bergeser cukup jauh.
                const float ratio = notchFrequency_[slot] > 0.0f
                                            ? frequency / notchFrequency_[slot]
                                            : 2.0f;
                if (ratio < 0.97f || ratio > 1.03f) {
                    notchFrequency_[slot] = frequency;
                    const float sr = static_cast<float>(sampleRate_);
                    for (int c = 0; c < kMaxChannels; ++c) {
                        // Perbarui koefisien tanpa me-reset keadaan filter:
                        // me-reset akan meneruskan transien dan menghilangkan
                        // efek notch.
                        notch_[slot][c].setNotch(sr, frequency, kNotchQ);
                    }
                }
                // Mendalam bertahap dalam waktu tetap (~0,5 detik) agar halus.
                const float step = kMaxDepth * static_cast<float>(frames) /
                                   static_cast<float>(attackFrames_);
                notchDepth_[slot] = std::fmin(kMaxDepth, notchDepth_[slot] + step);
            }
        }

        // Pelepasan sangat lambat setelah notch lama menganggur.
        for (int n = 0; n < kMaxNotches; ++n) {
            if (notchDepth_[n] <= 0.0f) continue;
            if (notchIdleFrames_[n] > releaseFrames_) {
                const float step = kMaxDepth * static_cast<float>(frames) /
                                   static_cast<float>(releaseFrames_);
                notchDepth_[n] -= step;
                if (notchDepth_[n] < 0.0f) notchDepth_[n] = 0.0f;
            }
        }
    }

    /** Frekuensi puncak yang dihaluskan dari energi pita di sekitar puncak. */
    float refineFrequency(int strongest) const {
        float frequency = center_[strongest];
        if (strongest > 0 && strongest < kBands - 1) {
            const float e0 = std::log(bandEnv_[strongest - 1] + 1e-9f);
            const float e1 = std::log(bandEnv_[strongest] + 1e-9f);
            const float e2 = std::log(bandEnv_[strongest + 1] + 1e-9f);
            const float denom = e0 - 2.0f * e1 + e2;
            if (std::fabs(denom) > 1e-12f) {
                float delta = 0.5f * (e0 - e2) / denom;
                if (delta > 0.5f) delta = 0.5f;
                if (delta < -0.5f) delta = -0.5f;
                const float spacing = std::log(center_[1] / center_[0]);
                frequency = center_[strongest] * std::exp(delta * spacing);
            }
        }
        return frequency;
    }

    int findNotchNear(float frequency) const {
        for (int n = 0; n < kMaxNotches; ++n) {
            if (notchDepth_[n] <= 0.0f || notchFrequency_[n] <= 0.0f) continue;
            const float ratio = notchFrequency_[n] / frequency;
            if (ratio > 0.85f && ratio < 1.18f) return n;
        }
        return -1;
    }

    int findFreeNotch() const {
        for (int n = 0; n < kMaxNotches; ++n) {
            if (notchDepth_[n] <= 0.0f) return n;
        }
        return -1;
    }

    static constexpr float kMinLevel = 0.06f;        // ambang level minimum (~-24 dBFS)
    static constexpr float kProminenceRatio = 5.0f;  // ~14 dB di atas median
    static constexpr float kNotchQ = 12.0f;          // notch sangat sempit
    static constexpr float kMaxDepth = 0.95f;        // kedalaman maksimum

    double sampleRate_ = 48000.0;
    int32_t channels_ = 1;
    float detectorQ_ = 3.0f;
    float envelopeCoeff_ = 0.0f;
    int32_t confirmFrames_ = 0;   // ~0,4 s kondisi kandidat sebelum notch
    int32_t attackFrames_ = 0;    // ~0,5 s untuk notch mencapai kedalaman penuh
    int32_t releaseFrames_ = 0;   // ~8 s untuk melepas notch
    float center_[kBands]{};
    StateVariableFilter detector_[kBands];
    float bandEnv_[kBands]{};

    Biquad notch_[kMaxNotches][kMaxChannels];
    float notchDepth_[kMaxNotches]{};
    float notchFrequency_[kMaxNotches]{};
    int32_t notchIdleFrames_[kMaxNotches]{};

    int candidateBand_ = -1;
    int32_t candidateFrames_ = 0;
    float previousCandidateEnv_ = 0.0f;
};

}  // namespace alto

#endif  // ALTOGUITARFX_ANTIFEEDBACK_H
