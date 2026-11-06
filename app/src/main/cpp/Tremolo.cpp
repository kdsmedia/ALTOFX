#include "Tremolo.h"

namespace alto {

Tremolo::Tremolo() : Effect(2, "Tremolo") {
    setParam(0, 5.0f);  // rate Hz
    setParam(1, 0.5f);  // depth
}

void Tremolo::prepare(double sampleRate, int32_t channels) {
    Effect::prepare(sampleRate, channels);
    rate_.configure(param(0), 20.0f, sampleRate);
    depth_.configure(param(1), 20.0f, sampleRate);
    lfo_.setSampleRate(sampleRate);
    lfo_.setRate(param(0));
    lfo_.reset();
}

void Tremolo::reset() {
    lfo_.reset();
}

void Tremolo::onParamChanged(int index, float value) {
    switch (index) {
        case 0: rate_.setTarget(value); break;
        case 1: depth_.setTarget(value); break;
        default: break;
    }
}

void Tremolo::process(float* buffer, int32_t frames, int32_t channels) {
    rate_.setTarget(param(0));
    depth_.setTarget(param(1));

    for (int32_t frame = 0; frame < frames; ++frame) {
        lfo_.setRate(rate_.next());
        const float depth = depth_.next();
        // LFO -1..1 dipetakan ke penguatan 1 (puncak) .. 1-depth (lembah).
        const float modulation = 0.5f + 0.5f * lfo_.next();
        const float gain = 1.0f - depth * (1.0f - modulation);
        for (int32_t channel = 0; channel < channels; ++channel) {
            buffer[frame * channels + channel] *= gain;
        }
    }
}

}  // namespace alto
