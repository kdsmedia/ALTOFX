#ifndef ALTOGUITARFX_AUTOWAH_H
#define ALTOGUITARFX_AUTOWAH_H

#include "Effect.h"
#include "StateVariableFilter.h"

namespace alto {

/**
 * Auto Wah (envelope wah) — efek wah yang dikendalikan dinamika permainan.
 *
 * Meniru pedal wah klasik (mis. Mu-Tron/Cry Baby) tanpa pedal ekspresi:
 * selubung (envelope) amplitudo sinyal gitar menggerakkan frekuensi puncak
 * filter band-pass. Petikan kuat membuka filter (terang), nada yang meluruh
 * menutupnya kembali (gelap). Memberi karakter funk/lead yang hidup.
 *
 * Parameter:
 *   0 sensitivity : 0..1  (seberapa jauh dinamika membuka filter)
 *   1 range       : 0..1  (lebar sapuan frekuensi)
 *   2 resonance   : 0.5..10 (ketajaman/peak filter, khas "wah")
 *   3 mix         : 0..1  (keseimbangan sinyal wah vs kering)
 *
 * Semua parameter dihaluskan per sampel (SmoothedParam) agar perubahan dari UI
 * tidak menimbulkan klik. Tidak ada alokasi memori di jalur audio.
 */
class AutoWah : public Effect {
public:
    static constexpr int kMaxChannels = 2;

    AutoWah();

    void prepare(double sampleRate, int32_t channels) override;
    void reset() override;
    void process(float* buffer, int32_t frames, int32_t channels) override;

protected:
    void onParamChanged(int index, float value) override;

private:
    static constexpr float kMinCutoff = 220.0f;
    static constexpr float kMaxCutoff = 2800.0f;
    static constexpr float kMakeup = 1.6f;

    SmoothedParam sensitivity_;
    SmoothedParam range_;
    SmoothedParam resonance_;
    SmoothedParam mix_;

    StateVariableFilter filter_[kMaxChannels];
    float envelope_ = 0.0f;
    float attackCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
};

}  // namespace alto

#endif  // ALTOGUITARFX_AUTOWAH_H
