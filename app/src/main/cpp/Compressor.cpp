#include "Compressor.h"

namespace alto {

namespace {
constexpr float kKneeDb = 6.0f;
constexpr float kEnvelopeMs = 10.0f;

float dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

float linearToDb(float linear) {
    return 20.0f * std::log10(linear > 1e-9f ? linear : 1e-9f);
}

float timeToCoefficient(float ms, double sampleRate) {
    if (ms <= 0.0f) {
        return 1.0f;
    }
    return 1.0f - std::exp(-1.0f / (ms * 0.001f * static_cast<float>(sampleRate)));
}
}  // namespace

Compressor::Compressor() : Effect(5, "Compressor") {
    setParam(0, -18.0f);  // threshold dB
    setParam(1, 4.0f);    // ratio
    setParam(2, 10.0f);   // attack ms
    setParam(3, 120.0f);  // release ms
    setParam(4, 6.0f);    // makeup dB
}

void Compressor::prepare(double sampleRate, int32_t channels) {
    Effect::prepare(sampleRate, channels);
    thresholdDb_.configure(param(0), 20.0f, sampleRate);
    ratio_.configure(param(1), 20.0f, sampleRate);
    attackMs_.configure(param(2), 20.0f, sampleRate);
    releaseMs_.configure(param(3), 20.0f, sampleRate);
    makeupDb_.configure(param(4), 20.0f, sampleRate);
}

void Compressor::reset() {
    envelope_ = 0.0f;
    gainDb_ = 0.0f;
}

void Compressor::onParamChanged(int index, float value) {
    switch (index) {
        case 0: thresholdDb_.setTarget(value); break;
        case 1: ratio_.setTarget(value); break;
        case 2: attackMs_.setTarget(value); break;
        case 3: releaseMs_.setTarget(value); break;
        case 4: makeupDb_.setTarget(value); break;
        default: break;
    }
}

void Compressor::process(float* buffer, int32_t frames, int32_t channels) {
    // Target dibaca sekali per blok, lalu dihaluskan per sampel.
    thresholdDb_.setTarget(param(0));
    ratio_.setTarget(param(1));
    attackMs_.setTarget(param(2));
    releaseMs_.setTarget(param(3));
    makeupDb_.setTarget(param(4));

    const float envelopeCoefficient = timeToCoefficient(kEnvelopeMs, sampleRate_);

    for (int32_t frame = 0; frame < frames; ++frame) {
        const float thresholdDb = thresholdDb_.next();
        const float ratio = ratio_.next();
        const float attackCoefficient = timeToCoefficient(attackMs_.next(), sampleRate_);
        const float releaseCoefficient = timeToCoefficient(releaseMs_.next(), sampleRate_);
        const float makeup = dbToLinear(makeupDb_.next());
        const float slope = 1.0f - 1.0f / (ratio > 1.0f ? ratio : 1.0f);

        // Selubung dari nilai maksimum antar-kanal (deteksi puncak halus).
        float peak = 0.0f;
        for (int32_t channel = 0; channel < channels; ++channel) {
            const float sample = buffer[frame * channels + channel];
            const float magnitude = (sample < 0.0f) ? -sample : sample;
            if (magnitude > peak) {
                peak = magnitude;
            }
        }
        envelope_ += (peak - envelope_) * envelopeCoefficient;

        const float inputDb = linearToDb(envelope_);
        // Kurva soft-knee: di bawah knee tidak ada kompresi, di dalam knee
        // transisi kuadratik, di atas knee rasio penuh.
        const float overDb = inputDb - thresholdDb;
        float reductionDb = 0.0f;
        if (overDb >= kKneeDb * 0.5f) {
            reductionDb = overDb * slope;
        } else if (overDb > -kKneeDb * 0.5f) {
            const float x = overDb + kKneeDb * 0.5f;
            reductionDb = slope * (x * x) / (2.0f * kKneeDb);
        }

        const float targetGainDb = -reductionDb;
        const float coefficient = (targetGainDb < gainDb_) ? attackCoefficient
                                                           : releaseCoefficient;
        gainDb_ += (targetGainDb - gainDb_) * coefficient;

        const float gain = dbToLinear(gainDb_) * makeup;
        for (int32_t channel = 0; channel < channels; ++channel) {
            buffer[frame * channels + channel] *= gain;
        }
    }
}

}  // namespace alto
