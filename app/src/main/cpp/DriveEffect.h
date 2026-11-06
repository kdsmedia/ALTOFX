#ifndef ALTOGUITARFX_DRIVEEFFECT_H
#define ALTOGUITARFX_DRIVEEFFECT_H

#include "Effect.h"
#include "Saturation.h"

namespace alto {

/**
 * Basis efek saturasi (BAB 8): Overdrive, Distortion, Fuzz.
 *
 * Semua memakai parameter yang sama (drive/gain, tone, level) dan struktur
 * yang sama: penguatan input -> waveshaping non-linear (ADAA) -> kontrol tone
 * -> level output, dengan DC blocker untuk mencegah offset.
 */
class DriveEffect : public Effect {
public:
    enum class Shape {
        SoftTanh,   // Overdrive: saturasi halus
        HardClip,   // Distortion: klip tegas
        Fuzz        // Fuzz: klip asimetris, gain tinggi
    };

    DriveEffect(const char* name, Shape shape, float driveScale);

    void prepare(double sampleRate, int32_t channels) override;
    void reset() override;
    void process(float* buffer, int32_t frames, int32_t channels) override;

protected:
    void onParamChanged(int index, float value) override;

private:
    Shape shape_;
    float driveScale_;
    SmoothedParam drive_;
    SmoothedParam tone_;
    SmoothedParam level_;
    AdaaSaturator saturator_;
    OnePoleLowPass toneFilter_;
    DcBlocker dcBlocker_;
};

}  // namespace alto

#endif  // ALTOGUITARFX_DRIVEEFFECT_H
