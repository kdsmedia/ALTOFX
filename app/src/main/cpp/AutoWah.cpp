#include "AutoWah.h"

#include <cmath>

namespace alto {

namespace {
// Selubung cepat pada serangan (mengikuti petikan) dan lambat saat meluruh.
constexpr float kAttackMs = 4.0f;
constexpr float kReleaseMs = 120.0f;
constexpr float kEnvelopeTarget = 0.6f;

float timeToCoefficient(float ms, double sampleRate) {
    if (ms <= 0.0f) return 1.0f;
    return 1.0f - std::exp(-1.0f / (ms * 0.001f * static_cast<float>(sampleRate)));
}
}  // namespace

AutoWah::AutoWah() : Effect(4, "Auto Wah") {
    setParam(0, 0.6f);   // sensitivity
    setParam(1, 0.7f);   // range
    setParam(2, 4.0f);   // resonance
    setParam(3, 0.9f);   // mix
}

void AutoWah::prepare(double sampleRate, int32_t channels) {
    Effect::prepare(sampleRate, channels);
    sensitivity_.configure(param(0), 20.0f, sampleRate);
    range_.configure(param(1), 20.0f, sampleRate);
    resonance_.configure(param(2), 20.0f, sampleRate);
    mix_.configure(param(3), 20.0f, sampleRate);
    attackCoeff_ = timeToCoefficient(kAttackMs, sampleRate);
    releaseCoeff_ = timeToCoefficient(kReleaseMs, sampleRate);
    for (int32_t c = 0; c < kMaxChannels; ++c) {
        filter_[c].setSampleRate(sampleRate);
        filter_[c].reset();
    }
    envelope_ = 0.0f;
}

void AutoWah::reset() {
    for (int32_t c = 0; c < kMaxChannels; ++c) {
        filter_[c].reset();
    }
    envelope_ = 0.0f;
}

void AutoWah::onParamChanged(int index, float value) {
    switch (index) {
        case 0: sensitivity_.setTarget(value); break;
        case 1: range_.setTarget(value); break;
        case 2: resonance_.setTarget(value); break;
        case 3: mix_.setTarget(value); break;
        default: break;
    }
}

void AutoWah::process(float* buffer, int32_t frames, int32_t channels) {
    sensitivity_.setTarget(param(0));
    range_.setTarget(param(1));
    resonance_.setTarget(param(2));
    mix_.setTarget(param(3));

    for (int32_t frame = 0; frame < frames; ++frame) {
        const float sensitivity = sensitivity_.next();
        const float range = range_.next();
        const float resonance = resonance_.next();
        const float mix = mix_.next();

        // Selubung dari nilai puncak antar-kanal (gitar mono).
        float peak = 0.0f;
        for (int32_t c = 0; c < channels; ++c) {
            const float sample = buffer[frame * channels + c];
            const float magnitude = sample < 0.0f ? -sample : sample;
            if (magnitude > peak) peak = magnitude;
        }
        const float coeff = (peak > envelope_) ? attackCoeff_ : releaseCoeff_;
        envelope_ += (peak - envelope_) * coeff;

        // Tingkat buka 0..1 dari selubung; sensitivity mengatur kepekaan.
        const float drive = envelope_ * (1.0f + sensitivity * 4.0f);
        float open = drive / kEnvelopeTarget;
        if (open > 1.0f) open = 1.0f;

        // Frekuensi potong bergerak dari dasar ke puncak sesuai range & open.
        const float low = kMinCutoff;
        const float high = low + (kMaxCutoff - low) * (0.25f + 0.75f * range);
        const float cutoff = low * std::pow(high / low, open);

        for (int32_t c = 0; c < channels; ++c) {
            const float dry = buffer[frame * channels + c];
            const float wet = filter_[c].bandPass(dry, cutoff, resonance);
            // Filter band-pass memiliki penguatan puncak ~Q (resonance). Bagi
            // dengan resonance agar tingkat keluaran tetap konsisten saat
            // ketajaman diubah, lalu beri sedikit dorongan khas wah.
            const float wetGain = mix * kMakeup / resonance;
            buffer[frame * channels + c] = dry * (1.0f - mix) + wet * wetGain;
        }
    }
}

}  // namespace alto
