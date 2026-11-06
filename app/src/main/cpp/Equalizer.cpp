#include "Equalizer.h"

namespace alto {

Equalizer::Equalizer() : Effect(3, "Equalizer") {
    setParam(0, 0.0f);  // bass dB
    setParam(1, 0.0f);  // mid dB
    setParam(2, 0.0f);  // treble dB
}

void Equalizer::prepare(double sampleRate, int32_t channels) {
    Effect::prepare(sampleRate, channels);
    // Waktu perataan cukup lama agar perubahan preset tidak menimbulkan klik.
    bassGain_.configure(param(0), 30.0f, sampleRate);
    midGain_.configure(param(1), 30.0f, sampleRate);
    trebleGain_.configure(param(2), 30.0f, sampleRate);
    for (auto& filter : bass_) filter.reset();
    for (auto& filter : mid_) filter.reset();
    for (auto& filter : treble_) filter.reset();
    updateCoefficients(bassGain_.current(), midGain_.current(), trebleGain_.current());
}

void Equalizer::onParamChanged(int index, float value) {
    switch (index) {
        case 0: bassGain_.setTarget(value); break;
        case 1: midGain_.setTarget(value); break;
        case 2: trebleGain_.setTarget(value); break;
        default: break;
    }
}

void Equalizer::reset() {
    for (auto& filter : bass_) filter.reset();
    for (auto& filter : mid_) filter.reset();
    for (auto& filter : treble_) filter.reset();
}

void Equalizer::updateCoefficients(float bassDb, float midDb, float trebleDb) {
    const float sampleRate = static_cast<float>(sampleRate_);
    for (int32_t channel = 0; channel < kMaxChannels; ++channel) {
        bass_[channel].setLowShelf(sampleRate, kBassFrequency, kShelfQ, bassDb);
        mid_[channel].setPeaking(sampleRate, kMidFrequency, kMidQ, midDb);
        treble_[channel].setHighShelf(sampleRate, kTrebleFrequency, kShelfQ, trebleDb);
    }
}

void Equalizer::process(float* buffer, int32_t frames, int32_t channels) {
    // Gain dihaluskan per sampel; koefisien dihitung ulang tiap 32 sampel agar
    // perubahan preset diterapkan bertahap tanpa lonjakan atau klik.
    for (int32_t frame = 0; frame < frames; ++frame) {
        // Gain dihaluskan tiap sampel; koefisien biquad dihitung ulang tiap
        // 32 sampel (cukup sering untuk perubahan mulus, hemat CPU).
        const float bassDb = bassGain_.next();
        const float midDb = midGain_.next();
        const float trebleDb = trebleGain_.next();
        if ((frame & 31) == 0) {
            updateCoefficients(bassDb, midDb, trebleDb);
        }
        for (int32_t channel = 0; channel < channels; ++channel) {
            float sample = buffer[frame * channels + channel];
            sample = bass_[channel].process(sample);
            sample = mid_[channel].process(sample);
            sample = treble_[channel].process(sample);
            buffer[frame * channels + channel] = sample;
        }
    }
}

}  // namespace alto
