#include "Chorus.h"

namespace alto {

Chorus::Chorus() : Effect(3, "Chorus") {
    setParam(0, 1.0f);  // rate Hz
    setParam(1, 0.4f);  // depth
    setParam(2, 0.5f);  // mix
}

void Chorus::prepare(double sampleRate, int32_t channels) {
    Effect::prepare(sampleRate, channels);
    rate_.configure(param(0), 20.0f, sampleRate);
    depth_.configure(param(1), 20.0f, sampleRate);
    mix_.configure(param(2), 20.0f, sampleRate);
    lfo_.setSampleRate(sampleRate);
    lfo_.setRate(param(0));
    lfo_.reset();
    delay_.prepare(sampleRate, kBaseDelayMs + kMaxModulationMs + 2.0f, kMaxChannels);
    delay_.reset();
}

void Chorus::reset() {
    lfo_.reset();
    delay_.reset();
}

void Chorus::onParamChanged(int index, float value) {
    switch (index) {
        case 0: rate_.setTarget(value); break;
        case 1: depth_.setTarget(value); break;
        case 2: mix_.setTarget(value); break;
        default: break;
    }
}

void Chorus::process(float* buffer, int32_t frames, int32_t channels) {
    rate_.setTarget(param(0));
    depth_.setTarget(param(1));
    mix_.setTarget(param(2));

    const float sampleRate = static_cast<float>(sampleRate_);

    for (int32_t frame = 0; frame < frames; ++frame) {
        lfo_.setRate(rate_.next());
        const float depth = depth_.next();
        const float mix = mix_.next();
        const float modulation = lfo_.next();

        for (int32_t channel = 0; channel < channels; ++channel) {
            // Polaritas LFO dibalik pada kanal kedua agar citra stereo melebar.
            const float lfoValue = (channel == 0) ? modulation : -modulation;
            const float delayMs = kBaseDelayMs + kMaxModulationMs * depth * lfoValue;
            const float delaySamples = delayMs * 0.001f * sampleRate;

            const float dry = buffer[frame * channels + channel];
            const float wet = delay_.read(delaySamples, channel);
            delay_.write(dry, channel);
            buffer[frame * channels + channel] = dry * (1.0f - mix) + wet * mix;
        }
        delay_.advance();
    }
}

}  // namespace alto
