#include "Flanger.h"

namespace alto {

Flanger::Flanger() : Effect(4, "Flanger") {
    setParam(0, 0.5f);  // rate Hz
    setParam(1, 0.5f);  // depth
    setParam(2, 0.5f);  // feedback
    setParam(3, 0.5f);  // mix
}

void Flanger::prepare(double sampleRate, int32_t channels) {
    Effect::prepare(sampleRate, channels);
    rate_.configure(param(0), 20.0f, sampleRate);
    depth_.configure(param(1), 20.0f, sampleRate);
    feedback_.configure(param(2), 20.0f, sampleRate);
    mix_.configure(param(3), 20.0f, sampleRate);
    lfo_.setSampleRate(sampleRate);
    lfo_.setRate(param(0));
    lfo_.reset();
    delay_.prepare(sampleRate, kBaseDelayMs + kMaxModulationMs + 2.0f, kMaxChannels);
    delay_.reset();
    for (auto& value : feedbackSample_) {
        value = 0.0f;
    }
}

void Flanger::reset() {
    lfo_.reset();
    delay_.reset();
    for (auto& value : feedbackSample_) {
        value = 0.0f;
    }
}

void Flanger::onParamChanged(int index, float value) {
    switch (index) {
        case 0: rate_.setTarget(value); break;
        case 1: depth_.setTarget(value); break;
        case 2: feedback_.setTarget(value); break;
        case 3: mix_.setTarget(value); break;
        default: break;
    }
}

void Flanger::process(float* buffer, int32_t frames, int32_t channels) {
    rate_.setTarget(param(0));
    depth_.setTarget(param(1));
    feedback_.setTarget(param(2));
    mix_.setTarget(param(3));

    const float sampleRate = static_cast<float>(sampleRate_);

    for (int32_t frame = 0; frame < frames; ++frame) {
        lfo_.setRate(rate_.next());
        const float depth = depth_.next();
        const float feedback = feedback_.next();
        const float mix = mix_.next();
        const float modulation = lfo_.next();

        for (int32_t channel = 0; channel < channels; ++channel) {
            const float lfoValue = (channel == 0) ? modulation : -modulation;
            const float delayMs = kBaseDelayMs + kMaxModulationMs * depth * (0.5f + 0.5f * lfoValue);
            const float delaySamples = delayMs * 0.001f * sampleRate;

            const float dry = buffer[frame * channels + channel];
            // Umpan balik dibatasi di bawah 1 agar tetap stabil.
            const float input = dry + feedbackSample_[channel] * feedback * 0.95f;
            const float wet = delay_.read(delaySamples, channel);
            delay_.write(input, channel);
            feedbackSample_[channel] = wet;
            buffer[frame * channels + channel] = dry * (1.0f - mix) + wet * mix;
        }
        delay_.advance();
    }
}

}  // namespace alto
