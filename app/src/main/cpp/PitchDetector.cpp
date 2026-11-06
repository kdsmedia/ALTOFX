#include "PitchDetector.h"

#include <cmath>

namespace alto {

void PitchDetector::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    reset();
}

void PitchDetector::reset() {
    fill_ = 0;
    frequency_.store(0.0f);
    midiNote_.store(-1.0f);
    cents_.store(0.0f);
    confidence_.store(0.0f);
}

void PitchDetector::process(const float* input, int32_t frames, int32_t channels) {
    if (!active_.load() || input == nullptr) {
        return;
    }

    for (int32_t frame = 0; frame < frames; ++frame) {
        buffer_[fill_++] = input[frame * channels];
        if (fill_ < kWindowSize) {
            continue;
        }

        float confidence = 0.0f;
        const float frequency = detect(confidence);
        fill_ = 0;

        if (frequency > 0.0f) {
            const float reference = referenceHz_.load();
            const float midi = 69.0f + 12.0f * std::log2(frequency / reference);
            const float nearest = std::round(midi);
            frequency_.store(frequency);
            midiNote_.store(nearest);
            cents_.store((midi - nearest) * 100.0f);
            confidence_.store(confidence);
        } else {
            frequency_.store(0.0f);
            midiNote_.store(-1.0f);
            cents_.store(0.0f);
            confidence_.store(0.0f);
        }
    }
}

float PitchDetector::detect(float& confidenceOut) {
    constexpr int32_t tauMax = kWindowSize / 2;

    // 1) Fungsi selisih YIN.
    for (int32_t tau = 1; tau <= tauMax; ++tau) {
        float sum = 0.0f;
        for (int32_t i = 0; i + tau < kWindowSize; ++i) {
            const float delta = buffer_[i] - buffer_[i + tau];
            sum += delta * delta;
        }
        difference_[tau] = sum;
    }

    // 2) Normalisasi kumulatif.
    cumulative_[0] = 1.0f;
    float running = 0.0f;
    for (int32_t tau = 1; tau <= tauMax; ++tau) {
        running += difference_[tau];
        cumulative_[tau] = (running > 0.0f) ? (difference_[tau] * static_cast<float>(tau) / running)
                                            : 1.0f;
    }

    // 3) Ambang batas absolut: cari titik terendah di bawah ambang, lalu pilih
    //    minimum lokal pertamanya.
    int32_t tauEstimate = -1;
    for (int32_t tau = 2; tau < tauMax; ++tau) {
        if (cumulative_[tau] < kThreshold) {
            while (tau + 1 < tauMax && cumulative_[tau + 1] < cumulative_[tau]) {
                ++tau;
            }
            tauEstimate = tau;
            break;
        }
    }

    // Bila tidak ada yang di bawah ambang, pakai minimum global (nada lemah).
    if (tauEstimate < 0) {
        int32_t best = 0;
        for (int32_t tau = 2; tau < tauMax; ++tau) {
            if (cumulative_[tau] < cumulative_[best]) {
                best = tau;
            }
        }
        if (cumulative_[best] < 0.5f) {
            tauEstimate = best;
        }
    }

    if (tauEstimate < 1) {
        confidenceOut = 0.0f;
        return 0.0f;
    }

    // 4) Interpolasi parabolik untuk presisi sub-sampel.
    float tau = static_cast<float>(tauEstimate);
    if (tauEstimate > 1 && tauEstimate + 1 <= tauMax) {
        const float y0 = cumulative_[tauEstimate - 1];
        const float y1 = cumulative_[tauEstimate];
        const float y2 = cumulative_[tauEstimate + 1];
        const float denominator = 2.0f * (2.0f * y1 - y2 - y0);
        if (std::fabs(denominator) > 1e-9f) {
            tau += (y2 - y0) / denominator;
        }
    }

    const float frequency = static_cast<float>(sampleRate_) / tau;
    confidenceOut = 1.0f - cumulative_[tauEstimate];
    if (frequency < kMinFrequency || frequency > kMaxFrequency) {
        return 0.0f;
    }
    return frequency;
}

}  // namespace alto
