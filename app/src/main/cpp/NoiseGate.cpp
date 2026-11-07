#include "NoiseGate.h"

namespace alto {

namespace {
constexpr float kMinDb = -80.0f;
constexpr float kMaxDb = 0.0f;
constexpr float kEnvelopeMs = 2.0f;   // Jendela deteksi selubung.
constexpr float kHoldMs = 90.0f;      // Tahan buka agar nada meluruh tidak terpotong.

float dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

float timeToCoefficient(float ms, double sampleRate) {
    if (ms <= 0.0f) {
        return 1.0f;
    }
    return 1.0f - std::exp(-1.0f / (ms * 0.001f * static_cast<float>(sampleRate)));
}
}  // namespace

NoiseGate::NoiseGate() : Effect(3, "Noise Gate") {
    // Nilai awal aman; parameter sesungguhnya diisi dari UI/preset.
    setParam(0, -50.0f);  // threshold dB
    setParam(1, 120.0f);  // release ms
    setParam(2, 5.0f);    // attack ms
    // Catatan: efek default nonaktif di level DSP (kontrak uji bypass). Aplikasi
    // menyalakannya secara bawaan lewat AudioController.init agar bising (hiss)
    // saat perangkat dicolok langsung ditekan.
}

void NoiseGate::prepare(double sampleRate, int32_t channels) {
    Effect::prepare(sampleRate, channels);
    thresholdDb_.configure(param(0), 20.0f, sampleRate);
    releaseMs_.configure(param(1), 20.0f, sampleRate);
    attackMs_.configure(param(2), 20.0f, sampleRate);
    envelopeCoefficient_ = timeToCoefficient(kEnvelopeMs, sampleRate);
    holdSamples_ = static_cast<int32_t>(kHoldMs * 0.001f * static_cast<float>(sampleRate));
    holdCounter_ = 0;
}

void NoiseGate::onParamChanged(int index, float value) {
    switch (index) {
        case 0: thresholdDb_.setTarget(value); break;
        case 1: releaseMs_.setTarget(value); break;
        case 2: attackMs_.setTarget(value); break;
        default: break;
    }
}

void NoiseGate::reset() {
    envelope_ = 0.0f;
    gain_ = 0.0f;
    gateOpen_ = false;
    holdCounter_ = 0;
}

void NoiseGate::process(float* buffer, int32_t frames, int32_t channels) {
    // Target dibaca sekali per blok dari parameter atomik, lalu dihaluskan
    // per sampel agar tidak ada klik.
    thresholdDb_.setTarget(param(0));
    releaseMs_.setTarget(param(1));
    attackMs_.setTarget(param(2));

    for (int32_t frame = 0; frame < frames; ++frame) {
        const float thresholdDb = thresholdDb_.next();
        const float releaseMs = releaseMs_.next();
        const float attackMs = attackMs_.next();

        const float threshold = dbToLinear(thresholdDb);
        const float openThreshold = threshold;
        const float closeThreshold = threshold * 0.707f;  // histeresis ~3 dB
        const float attackCoefficient = timeToCoefficient(attackMs, sampleRate_);
        const float releaseCoefficient = timeToCoefficient(releaseMs, sampleRate_);

        // Deteksi selubung dari nilai maksimum antar-kanal.
        float peak = 0.0f;
        for (int32_t channel = 0; channel < channels; ++channel) {
            const float sample = buffer[frame * channels + channel];
            const float magnitude = (sample < 0.0f) ? -sample : sample;
            if (magnitude > peak) {
                peak = magnitude;
            }
        }
        envelope_ += (peak - envelope_) * envelopeCoefficient_;

        // Histeresis + waktu tahan: buka di atas ambang buka; setelah sinyal
        // turun di bawah ambang tutup, tunggu hold sebelum benar-benar menutup.
        // Ini mencegah gate memotong nada yang sedang meluruh (khas gitar) dan
        // mencegah chatter pada sela gelombang nada rendah.
        if (envelope_ >= openThreshold) {
            gateOpen_ = true;
            holdCounter_ = holdSamples_;
        } else if (gateOpen_ && envelope_ <= closeThreshold) {
            if (holdCounter_ > 0) {
                --holdCounter_;
            } else {
                gateOpen_ = false;
            }
        }

        const float targetGain = gateOpen_ ? 1.0f : 0.0f;
        const float coefficient = (targetGain > gain_) ? attackCoefficient
                                                       : releaseCoefficient;
        gain_ += (targetGain - gain_) * coefficient;

        for (int32_t channel = 0; channel < channels; ++channel) {
            buffer[frame * channels + channel] *= gain_;
        }
    }
}

}  // namespace alto
