#include "Delay.h"

namespace alto {

Delay::Delay() : Effect(3, "Delay") {
    setParam(0, 300.0f);  // time ms
    setParam(1, 0.4f);    // feedback
    setParam(2, 0.3f);    // mix
}

void Delay::prepare(double sampleRate, int32_t channels) {
    Effect::prepare(sampleRate, channels);
    timeMs_.configure(param(0), 50.0f, sampleRate);
    feedback_.configure(param(1), 20.0f, sampleRate);
    mix_.configure(param(2), 20.0f, sampleRate);
    delay_.prepare(sampleRate, kMaxDelayMs + 10.0f, kMaxChannels);
    delay_.reset();
}

void Delay::reset() {
    delay_.reset();
}

void Delay::onParamChanged(int index, float value) {
    switch (index) {
        case 0: timeMs_.setTarget(value); break;
        case 1: feedback_.setTarget(value); break;
        case 2: mix_.setTarget(value); break;
        default: break;
    }
}

void Delay::process(float* buffer, int32_t frames, int32_t channels) {
    timeMs_.setTarget(param(0));
    feedback_.setTarget(param(1));
    mix_.setTarget(param(2));

    const float sampleRate = static_cast<float>(sampleRate_);

    for (int32_t frame = 0; frame < frames; ++frame) {
        const float delaySamples = timeMs_.next() * 0.001f * sampleRate;
        const float feedback = feedback_.next();
        const float mix = mix_.next();

        for (int32_t channel = 0; channel < channels; ++channel) {
            const float dry = buffer[frame * channels + channel];
            const float wet = delay_.read(delaySamples, channel);
            // Umpan balik dibatasi di bawah 1 agar tidak meledak.
            delay_.write(dry + wet * feedback * 0.95f, channel);
            buffer[frame * channels + channel] = dry * (1.0f - mix) + wet * mix;
        }
        delay_.advance();
    }
}

}  // namespace alto
