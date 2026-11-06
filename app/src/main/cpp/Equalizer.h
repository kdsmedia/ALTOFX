#ifndef ALTOGUITARFX_EQUALIZER_H
#define ALTOGUITARFX_EQUALIZER_H

#include <array>

#include "Biquad.h"
#include "Effect.h"

namespace alto {

/**
 * Equalizer 3-band (BAB 9): low shelf (bass), peaking (mid), high shelf
 * (treble). Koefisien dihitung ulang di audio thread sekali per blok dari
 * nilai yang sudah dihaluskan, sehingga tidak ada data race dengan UI.
 */
class Equalizer : public Effect {
public:
    static constexpr float kBassFrequency = 120.0f;
    static constexpr float kMidFrequency = 800.0f;
    static constexpr float kTrebleFrequency = 3200.0f;
    static constexpr float kShelfQ = 0.707f;
    static constexpr float kMidQ = 1.0f;
    static constexpr int32_t kMaxChannels = 2;

    Equalizer();

    void prepare(double sampleRate, int32_t channels) override;
    void reset() override;
    void process(float* buffer, int32_t frames, int32_t channels) override;

private:
    void updateCoefficients(float bassDb, float midDb, float trebleDb);

    std::array<Biquad, kMaxChannels> bass_;
    std::array<Biquad, kMaxChannels> mid_;
    std::array<Biquad, kMaxChannels> treble_;
};

}  // namespace alto

#endif  // ALTOGUITARFX_EQUALIZER_H
