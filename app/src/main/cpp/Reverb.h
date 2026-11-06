#ifndef ALTOGUITARFX_REVERB_H
#define ALTOGUITARFX_REVERB_H

#include <array>
#include <cstdint>
#include <vector>

#include "Effect.h"

namespace alto {

/**
 * Reverb (BAB 11): jaringan Schroeder/Freeverb. Delapan comb filter berumpan
 * balik dengan peredam (damping) paralel, lalu empat allpass berurutan untuk
 * menyebarkan gema. Parameter: Size (ruang), Decay, Damping, Mix.
 *
 * Jumlah kombinasi dan waktu tunda mengikuti konstanta Freeverb asli; stereo
 * memakai penyebaran (spread) antar kanal.
 */
class Reverb : public Effect {
public:
    static constexpr int32_t kCombs = 8;
    static constexpr int32_t kAllPasses = 4;
    static constexpr int32_t kMaxChannels = 2;
    static constexpr float kStereoSpread = 23.0f;

    Reverb();

    void prepare(double sampleRate, int32_t channels) override;
    void reset() override;
    void process(float* buffer, int32_t frames, int32_t channels) override;

protected:
    void onParamChanged(int index, float value) override;

private:
    /** Comb filter berumpan balik dengan low-pass satu kutub (damping). */
    struct Comb {
        std::vector<float> buffer;
        int32_t index = 0;
        float filterState = 0.0f;

        void setSize(int32_t size) {
            buffer.assign(static_cast<size_t>(size), 0.0f);
            index = 0;
            filterState = 0.0f;
        }

        float process(float input, float feedback, float damping) {
            const float output = buffer[static_cast<size_t>(index)];
            filterState = output * (1.0f - damping) + filterState * damping;
            buffer[static_cast<size_t>(index)] = input + filterState * feedback;
            if (++index >= static_cast<int32_t>(buffer.size())) {
                index = 0;
            }
            return output;
        }
    };

    /** Allpass filter untuk penyebaran. */
    struct AllPass {
        std::vector<float> buffer;
        int32_t index = 0;

        void setSize(int32_t size) {
            buffer.assign(static_cast<size_t>(size), 0.0f);
            index = 0;
        }

        float process(float input) {
            const float buffered = buffer[static_cast<size_t>(index)];
            const float output = buffered - input;
            buffer[static_cast<size_t>(index)] = input + buffered * 0.5f;
            if (++index >= static_cast<int32_t>(buffer.size())) {
                index = 0;
            }
            return output;
        }
    };

    SmoothedParam mix_;
    std::array<std::array<Comb, kCombs>, kMaxChannels> combs_;
    std::array<std::array<AllPass, kAllPasses>, kMaxChannels> allPasses_;
};

}  // namespace alto

#endif  // ALTOGUITARFX_REVERB_H
