#ifndef ALTOGUITARFX_DRIVEEFFECT_H
#define ALTOGUITARFX_DRIVEEFFECT_H

#include "Biquad.h"
#include "Effect.h"
#include "Oversampling.h"
#include "Saturation.h"

namespace alto {

/**
 * Basis efek saturasi (BAB 8): Overdrive, Distortion, Fuzz.
 *
 * Struktur: penguatan input -> waveshaping non-linear (ADAA) -> kontrol tone
 * -> level output, dengan DC blocker per kanal.
 *
 * Untuk menghindari suara "pecah" (aliasing) pada gain tinggi, tahap klip
 * dijalankan pada laju 2x (oversampling) lalu disaring sebelum turun kembali.
 * Setiap filter bersifat per kanal agar tidak ada crosstalk antar kanal.
 */
class DriveEffect : public Effect {
public:
    enum class Shape {
        SoftTanh,   // Overdrive: saturasi halus
        HardClip,   // Distortion: klip tegas
        Fuzz        // Fuzz: klip asimetris, gain tinggi
    };

    static constexpr int kMaxChannels = 2;

    DriveEffect(const char* name, Shape shape, float driveScale);

    void prepare(double sampleRate, int32_t channels) override;
    void reset() override;
    void process(float* buffer, int32_t frames, int32_t channels) override;

protected:
    void onParamChanged(int index, float value) override;

private:
    void updateToneFilter(float tone);

    Shape shape_;
    float driveScale_;
    SmoothedParam drive_;
    SmoothedParam tone_;
    SmoothedParam level_;
    AdaaSaturator saturator_;
    Oversampler2x oversampler_;
    Biquad toneFilter_[kMaxChannels];
    DcBlocker dcBlocker_[kMaxChannels];
};

}  // namespace alto

#endif  // ALTOGUITARFX_DRIVEEFFECT_H
