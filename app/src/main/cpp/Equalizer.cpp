#include "Equalizer.h"

namespace alto {

Equalizer::Equalizer() : Effect(3, "Equalizer") {
    setParam(0, 0.0f);  // bass dB
    setParam(1, 0.0f);  // mid dB
    setParam(2, 0.0f);  // treble dB
}

void Equalizer::prepare(double sampleRate, int32_t channels) {
    Effect::prepare(sampleRate, channels);
    for (auto& filter : bass_) filter.reset();
    for (auto& filter : mid_) filter.reset();
    for (auto& filter : treble_) filter.reset();
    updateCoefficients(param(0), param(1), param(2));
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
    // Koefisien dihitung ulang dari nilai parameter atomik sekali per blok,
    // di audio thread. Nilai berubah hanya saat pengguna menggeser slider,
    // sehingga tidak menimbulkan klik yang terdengar.
    updateCoefficients(param(0), param(1), param(2));

    for (int32_t frame = 0; frame < frames; ++frame) {
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
