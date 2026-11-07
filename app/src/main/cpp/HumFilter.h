#ifndef ALTOGUITARFX_HUMFILTER_H
#define ALTOGUITARFX_HUMFILTER_H

#include <cstdint>

#include "Biquad.h"
#include "Saturation.h"

namespace alto {

/**
 * Peredam dengung (hum) dan derau frekuensi rendah pada jalur input.
 *
 * Grounding yang buruk, kabel gitar panjang, atau catu daya menghasilkan
 * dengung pada 50 Hz (dan harmonisanya 100/150 Hz) atau 60 Hz (120/180 Hz).
 * Blok ini memasang notch sempit pada frekuensi tersebut, membuang DC, dan
 * menyaring frekuensi di bawah rentang gitar (high-pass) sehingga efek
 * saturasi tidak menguatkan derau frekuensi rendah.
 *
 * Tidak ada alokasi di jalur audio; hanya biquad dan filter satu kutub.
 */
class HumFilter {
public:
    void prepare(double sampleRate, int32_t channels) {
        channels_ = channels > 0 ? channels : 1;
        if (channels_ > kMaxChannels) channels_ = kMaxChannels;
        const float sr = static_cast<float>(sampleRate);

        // Notch pada frekuensi jala-jala dan harmonisa ganjilnya (50/100/150 Hz
        // atau 60/120/180 Hz). Harmonisa ketiga (150 Hz) sering paling kuat dan
        // berada di rentang rendah gitar, jadi harus dibuang agar noise gate
        // dapat menutup saat diam dan distorsi tidak "mendengung". Dua biquad
        // dijatuhkan bertingkat (cascade) agar redaman jauh lebih dalam — satu
        // notch saja hanya menekan ~30 dB, tidak cukup karena efek distorsi
        // menguatkan sisa dengung puluhan kali.
        const float base = humHz_;
        for (int32_t c = 0; c < channels_; ++c) {
            notch50_[c].setNotch(sr, base, 30.0f);
            notch50b_[c].setNotch(sr, base, 30.0f);
            notch100_[c].setNotch(sr, base * 2.0f, 30.0f);
            notch100b_[c].setNotch(sr, base * 2.0f, 30.0f);
            notch150_[c].setNotch(sr, base * 3.0f, 30.0f);
            notch150b_[c].setNotch(sr, base * 3.0f, 30.0f);
            highPass_[c].setHighPass(sr, kHighPassHz, 0.707f);
        }
        reset();
    }

    /**
     * Mengatur frekuensi dengung dasar (50 Hz untuk jala 50 Hz, 60 Hz untuk
     * jala 60 Hz). Bila berubah, koefisien dihitung ulang; aman dipanggil dari
     * thread UI (bukan dari callback audio).
     */
    void setHumFrequency(float hz) {
        if (hz < 20.0f) hz = 20.0f;
        if (hz > 120.0f) hz = 120.0f;
        if (hz == humHz_) return;
        humHz_ = hz;
        const float sr = static_cast<float>(sampleRate_);
        for (int32_t c = 0; c < channels_; ++c) {
            notch50_[c].setNotch(sr, humHz_, 30.0f);
            notch50b_[c].setNotch(sr, humHz_, 30.0f);
            notch100_[c].setNotch(sr, humHz_ * 2.0f, 30.0f);
            notch100b_[c].setNotch(sr, humHz_ * 2.0f, 30.0f);
            notch150_[c].setNotch(sr, humHz_ * 3.0f, 30.0f);
            notch150b_[c].setNotch(sr, humHz_ * 3.0f, 30.0f);
        }
    }

    void setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }

    void reset() {
        for (int32_t c = 0; c < kMaxChannels; ++c) {
            notch50_[c].reset();
            notch50b_[c].reset();
            notch100_[c].reset();
            notch100b_[c].reset();
            notch150_[c].reset();
            notch150b_[c].reset();
            highPass_[c].reset();
            dcBlocker_[c].reset();
        }
    }

    void process(float* buffer, int32_t frames, int32_t channels) {
        const int32_t ch = channels < channels_ ? channels : channels_;
        for (int32_t frame = 0; frame < frames; ++frame) {
            for (int32_t c = 0; c < ch; ++c) {
                float& sample = buffer[frame * channels + c];
                sample = dcBlocker_[c].process(sample);
                sample = highPass_[c].process(sample);
                sample = notch50_[c].process(sample);
                sample = notch50b_[c].process(sample);
                sample = notch100_[c].process(sample);
                sample = notch100b_[c].process(sample);
                sample = notch150_[c].process(sample);
                sample = notch150b_[c].process(sample);
            }
        }
    }

private:
    static constexpr int32_t kMaxChannels = 2;
    static constexpr float kHighPassHz = 40.0f;

    double sampleRate_ = 48000.0;
    int32_t channels_ = 1;
    float humHz_ = 50.0f;
    Biquad notch50_[kMaxChannels];
    Biquad notch50b_[kMaxChannels];
    Biquad notch100_[kMaxChannels];
    Biquad notch100b_[kMaxChannels];
    Biquad notch150_[kMaxChannels];
    Biquad notch150b_[kMaxChannels];
    Biquad highPass_[kMaxChannels];
    DcBlocker dcBlocker_[kMaxChannels];
};

}  // namespace alto

#endif  // ALTOGUITARFX_HUMFILTER_H
