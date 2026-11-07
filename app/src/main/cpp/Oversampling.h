#ifndef ALTOGUITARFX_OVERSAMPLING_H
#define ALTOGUITARFX_OVERSAMPLING_H

#include <cstdint>

#include "Biquad.h"

namespace alto {

/**
 * Pembungkus oversampling 2x untuk tahap non-linear (saturasi/klip).
 *
 * Waveshaping non-linear menghasilkan harmonisa di atas Nyquist yang, tanpa
 * penanganan, "terlipat" kembali menjadi derau tajam (aliasing) yang terdengar
 * kasar/pecah pada distorsi gain tinggi. Dengan memproses klip pada laju 2x
 * lalu menyaring sebelum turun kembali, harmonisa liar ditekan sehingga
 * distorsi terdengar halus dan profesional — bukan "pecah".
 *
 * Cara pakai per sampel input:
 *   over.up(x, ch, a, b);     // dua sampel pada 2x
 *   a = f(a); b = f(b);       // terapkan fungsi non-linear
 *   y = over.down(a, b, ch);  // kembali ke 1x
 */
class Oversampler2x {
public:
    static constexpr int kMaxChannels = 2;

    /** [nyquistFraction] = batas pita sebagai pecahan Nyquist 1x (mis. 0.9). */
    void prepare(double sampleRate, int32_t channels, float nyquistFraction = 0.90f) {
        sampleRate2x_ = sampleRate * 2.0;
        channels_ = channels > kMaxChannels ? kMaxChannels : (channels < 1 ? 1 : channels);
        const float sr2x = static_cast<float>(sampleRate2x_);
        float cutoff = nyquistFraction * static_cast<float>(sampleRate) * 0.5f;
        const float maxCutoff = sr2x * 0.45f;
        if (cutoff > maxCutoff) cutoff = maxCutoff;
        if (cutoff < 200.0f) cutoff = 200.0f;
        for (int32_t c = 0; c < kMaxChannels; ++c) {
            up_[c].setLowPass(sr2x, cutoff, 0.7071f);
            down_[c].setLowPass(sr2x, cutoff, 0.7071f);
            up_[c].reset();
            down_[c].reset();
        }
    }

    void reset() {
        for (int32_t c = 0; c < kMaxChannels; ++c) {
            up_[c].reset();
            down_[c].reset();
        }
    }

    /** Zero-stuffing + penyaring interpolasi. Mengisi [out0] dan [out1]. */
    inline void up(float x, int32_t ch, float& out0, float& out1) {
        const int32_t c = ch < channels_ ? ch : (channels_ - 1);
        // Zero-stuffing melemahkan amplitudo; kompensasi dengan faktor 2.
        out0 = 2.0f * up_[c].process(x);
        out1 = 2.0f * up_[c].process(0.0f);
    }

    /** Penyaring dekimation lalu ambil satu sampel (buang yang kedua). */
    inline float down(float in0, float in1, int32_t ch) {
        const int32_t c = ch < channels_ ? ch : (channels_ - 1);
        down_[c].process(in1);
        return down_[c].process(in0);
    }

private:
    double sampleRate2x_ = 96000.0;
    int32_t channels_ = 1;
    Biquad up_[kMaxChannels];
    Biquad down_[kMaxChannels];
};

}  // namespace alto

#endif  // ALTOGUITARFX_OVERSAMPLING_H
