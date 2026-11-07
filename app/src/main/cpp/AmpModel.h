#ifndef ALTOGUITARFX_AMPMODEL_H
#define ALTOGUITARFX_AMPMODEL_H

#include <array>

#include "Biquad.h"
#include "Effect.h"
#include "Saturation.h"

namespace alto {

/**
 * Model amplifier gitar bergaya Tonebridge (BAB 8 lanjutan).
 *
 * Meniru jalur sinyal amplifier nyata: penguatan input (input stage) ->
 * saturasi preamp (ADAA) -> tone stack bass/mid/treble -> presence (high shelf)
 * -> simulasi kabinet (low-pass) -> master volume. Tiga voicing meniru
 * karakter amp bersih, crunch, dan high-gain.
 *
 * Seluruh parameter dihaluskan per sampel (SmoothedParam) sehingga perubahan
 * besar (mis. memuat preset) tidak menimbulkan klik.
 */
class AmpModel : public Effect {
public:
    /** Karakter amplifier. */
    enum class Voicing : int {
        Clean = 0,
        Crunch = 1,
        Lead = 2
    };

    static constexpr int kMaxChannels = 2;

    AmpModel();

    void prepare(double sampleRate, int32_t channels) override;
    void reset() override;
    void process(float* buffer, int32_t frames, int32_t channels) override;

private:
    void onParamChanged(int index, float value) override;
    void updateToneCoefficients(float bassDb, float midDb, float trebleDb, float presenceDb);

    SmoothedParam voicing_;   // disimpan sebagai float (indeks voicing)
    SmoothedParam gain_;
    SmoothedParam bass_;
    SmoothedParam mid_;
    SmoothedParam treble_;
    SmoothedParam presence_;
    SmoothedParam master_;

    AdaaSaturator saturator_;
    DcBlocker dcBlocker_;
    // Pemotong DC masukan (high-pass) untuk membuang frekuensi sangat rendah.
    Biquad inputHighPass_[kMaxChannels];
    Biquad bassFilter_[kMaxChannels];
    Biquad midFilter_[kMaxChannels];
    Biquad trebleFilter_[kMaxChannels];
    Biquad presenceFilter_[kMaxChannels];
    // Simulasi kabinet: low-pass untuk meredam fizz di atas frekuensi speaker.
    Biquad cabinet_[kMaxChannels];
};

}  // namespace alto

#endif  // ALTOGUITARFX_AMPMODEL_H
