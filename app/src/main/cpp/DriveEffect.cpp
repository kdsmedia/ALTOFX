#include "DriveEffect.h"

#include <cmath>

namespace alto {

namespace {
// Rentang kontrol tone: dari gelap ke terang.
constexpr float kToneLowHz = 600.0f;
constexpr float kToneHighHz = 9000.0f;
constexpr float kToneQ = 0.707f;

// High-pass jalur efek. Membuang rumble/dengung frekuensi rendah SEBELUM
// saturasi; tanpa ini, dengung (50/60 Hz dan harmonisanya) dan getaran
// frekuensi rendah ikut dikuatkan puluhan kali sehingga terdengar "berisik".
constexpr float kInputHighPassHz = 80.0f;
constexpr float kOutputHighPassHz = 70.0f;
constexpr float kHighPassQ = 0.707f;

inline float toneToCutoff(float tone) {
    float t = tone < 0.0f ? 0.0f : (tone > 1.0f ? 1.0f : tone);
    return kToneLowHz * std::pow(kToneHighHz / kToneLowHz, t);
}
}  // namespace

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
    oversampler_.prepare(sampleRate, channels);
    updateToneFilter(param(1));
    for (int32_t c = 0; c < kMaxChannels; ++c) {
        inputHighPass_[c].setHighPass(static_cast<float>(sampleRate), kInputHighPassHz, kHighPassQ);
        outputHighPass_[c].setHighPass(static_cast<float>(sampleRate), kOutputHighPassHz, kHighPassQ);
        toneFilter_[c].reset();
        inputHighPass_[c].reset();
        outputHighPass_[c].reset();
        dcBlocker_[c].reset();
    }
}

void DriveEffect::reset() {
    saturator_.reset();
    oversampler_.reset();
    for (int32_t c = 0; c < kMaxChannels; ++c) {
        toneFilter_[c].reset();
        inputHighPass_[c].reset();
        outputHighPass_[c].reset();
        dcBlocker_[c].reset();
    }
}

void DriveEffect::updateToneFilter(float tone) {
    const float cutoff = toneToCutoff(tone);
    for (int32_t c = 0; c < kMaxChannels; ++c) {
        toneFilter_[c].setLowPass(static_cast<float>(sampleRate_), cutoff, kToneQ);
    }
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
        // Taper eksponensial: gain = scale^drive. Memberi rentang bersih yang
        // berguna di setengah bawah knob (drive 0 nyaris bersih, drive 1 jenuh
        // penuh), alih-alih terklip sejak setelan kecil seperti taper linier.
        const float gain = std::pow(driveScale_, drive);
        if ((frame & 31) == 0) {
            updateToneFilter(tone);
        }

        for (int32_t channel = 0; channel < channels; ++channel) {
            // High-pass sebelum saturasi: buang rumble/dengung frekuensi rendah
            // agar tidak dikuatkan puluhan kali menjadi "berisik".
            float sample = inputHighPass_[channel].process(buffer[frame * channels + channel]);
            sample *= gain;

            // Waveshaping pada laju 2x untuk menekan aliasing (anti "pecah").
            float a = 0.0f;
            float b = 0.0f;
            oversampler_.up(sample, channel, a, b);
            switch (shape_) {
                case Shape::SoftTanh:
                    a = saturator_.processTanh(a, channel);
                    b = saturator_.processTanh(b, channel);
                    break;
                case Shape::HardClip:
                    a = saturator_.processHardClip(a, 1.0f, channel);
                    b = saturator_.processHardClip(b, 1.0f, channel);
                    break;
                case Shape::Fuzz:
                    a = saturator_.processAsymmetricClip(a, 0.7f, 1.0f, channel);
                    b = saturator_.processAsymmetricClip(b, 0.7f, 1.0f, channel);
                    break;
            }
            sample = oversampler_.down(a, b, channel);

            sample = toneFilter_[channel].process(sample);
            sample = dcBlocker_[channel].process(sample);
            // High-pass keluaran membuang sisa frekuensi rendah/dengung yang
            // timbul dari proses saturasi (intermodulasi).
            sample = outputHighPass_[channel].process(sample);
            buffer[frame * channels + channel] = sample * level;
        }
    }
}

}  // namespace alto
