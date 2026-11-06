#include "DriveEffect.h"

namespace alto {

DriveEffect::DriveEffect(const char* name, Shape shape, float driveScale)
        : Effect(3, name), shape_(shape), driveScale_(driveScale) {
    setParam(0, 0.4f);  // drive
    setParam(1, 0.5f);  // tone
    setParam(2, 0.7f);  // level
}

void DriveEffect::prepare(double sampleRate, int32_t channels) {
    Effect::prepare(sampleRate, channels);
    drive_.configure(param(0), 20.0f, sampleRate);
    tone_.configure(param(1), 20.0f, sampleRate);
    level_.configure(param(2), 20.0f, sampleRate);
    saturator_.reset();
    toneFilter_.reset();
    dcBlocker_.reset();
    toneFilter_.setCutoff(param(1));
}

void DriveEffect::reset() {
    saturator_.reset();
    toneFilter_.reset();
    dcBlocker_.reset();
}

void DriveEffect::onParamChanged(int index, float value) {
    switch (index) {
        case 0: drive_.setTarget(value); break;
        case 1: tone_.setTarget(value); break;
        case 2: level_.setTarget(value); break;
        default: break;
    }
}

void DriveEffect::process(float* buffer, int32_t frames, int32_t channels) {
    drive_.setTarget(param(0));
    tone_.setTarget(param(1));
    level_.setTarget(param(2));

    for (int32_t frame = 0; frame < frames; ++frame) {
        const float drive = drive_.next();
        const float tone = tone_.next();
        const float level = level_.next();
        const float gain = 1.0f + drive * driveScale_;
        toneFilter_.setCutoff(tone);

        for (int32_t channel = 0; channel < channels; ++channel) {
            float sample = buffer[frame * channels + channel];
            sample *= gain;

            switch (shape_) {
                case Shape::SoftTanh:
                    sample = saturator_.processTanh(sample, channel);
                    break;
                case Shape::HardClip:
                    sample = saturator_.processHardClip(sample, 1.0f, channel);
                    break;
                case Shape::Fuzz: {
                    // Klip asimetris (ambang positif/negatif berbeda) memberi
                    // harmonisa genap khas fuzz yang bertahan setelah DC hilang.
                    sample = saturator_.processAsymmetricClip(sample, 0.7f, 1.0f, channel);
                    break;
                }
            }

            sample = toneFilter_.process(sample);
            sample = dcBlocker_.process(sample);
            buffer[frame * channels + channel] = sample * level;
        }
    }
}

}  // namespace alto
