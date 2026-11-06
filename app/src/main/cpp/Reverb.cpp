#include "Reverb.h"

#include <algorithm>

namespace alto {

namespace {
// Konstanta waktu tunda Freeverb (pada 44,1 kHz), dalam sampel.
constexpr int32_t kCombTuning[Reverb::kCombs] = {
    1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617
};
constexpr int32_t kAllPassTuning[Reverb::kAllPasses] = {556, 441, 341, 225};
constexpr float kFixedGain = 0.015f;
constexpr float kReferenceRate = 44100.0f;
}  // namespace

Reverb::Reverb() : Effect(4, "Reverb") {
    setParam(0, 0.5f);  // size
    setParam(1, 0.5f);  // decay
    setParam(2, 0.5f);  // damping
    setParam(3, 0.3f);  // mix
}

void Reverb::prepare(double sampleRate, int32_t channels) {
    Effect::prepare(sampleRate, channels);
    mix_.configure(param(3), 20.0f, sampleRate);

    // Skala waktu tunda bila sample rate bukan 44,1 kHz.
    const float scale = static_cast<float>(sampleRate) / kReferenceRate;
    for (int32_t channel = 0; channel < kMaxChannels; ++channel) {
        const float spread = kStereoSpread * static_cast<float>(channel);
        for (int32_t i = 0; i < kCombs; ++i) {
            const int32_t size = static_cast<int32_t>(
                    (static_cast<float>(kCombTuning[i]) + spread) * scale);
            combs_[channel][i].setSize(size);
        }
        for (int32_t i = 0; i < kAllPasses; ++i) {
            const int32_t size = static_cast<int32_t>(
                    (static_cast<float>(kAllPassTuning[i]) + spread) * scale);
            allPasses_[channel][i].setSize(size);
        }
    }
}

void Reverb::reset() {
    for (auto& channel : combs_) {
        for (auto& comb : channel) {
            std::fill(comb.buffer.begin(), comb.buffer.end(), 0.0f);
            comb.index = 0;
            comb.filterState = 0.0f;
        }
    }
    for (auto& channel : allPasses_) {
        for (auto& allPass : channel) {
            std::fill(allPass.buffer.begin(), allPass.buffer.end(), 0.0f);
            allPass.index = 0;
        }
    }
}

void Reverb::onParamChanged(int index, float value) {
    if (index == 3) {
        mix_.setTarget(value);
    }
}

void Reverb::process(float* buffer, int32_t frames, int32_t channels) {
    // Parameter dibaca sekali per blok. Perubahan size/decay/damping tidak
    // menimbulkan klik karena hanya mengubah umpan balik dan peredaman.
    const float size = param(0);
    const float decay = param(1);
    const float dampingParam = param(2);
    mix_.setTarget(param(3));

    // Umpan balik comb: size dan decay bersama menentukan panjang ekor.
    // Rentang 0,68..0,98 (di bawah 1 agar selalu stabil).
    const float feedback = 0.68f + 0.30f * (0.4f * size + 0.6f * decay);
    // Peredaman: 0,05..0,45; makin tinggi makin cepat nada tinggi diserap.
    const float damping = 0.05f + dampingParam * 0.40f;

    for (int32_t frame = 0; frame < frames; ++frame) {
        const float mix = mix_.next();
        for (int32_t channel = 0; channel < channels; ++channel) {
            const int32_t reverbChannel = (channel < kMaxChannels) ? channel : 0;
            const float dry = buffer[frame * channels + channel];
            const float input = dry * kFixedGain;

            float wet = 0.0f;
            for (int32_t i = 0; i < kCombs; ++i) {
                wet += combs_[reverbChannel][i].process(input, feedback, damping);
            }
            for (int32_t i = 0; i < kAllPasses; ++i) {
                wet = allPasses_[reverbChannel][i].process(wet);
            }

            buffer[frame * channels + channel] = dry * (1.0f - mix) + wet * mix;
        }
    }
}

}  // namespace alto
