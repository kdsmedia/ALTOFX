#include "AmpModel.h"

#include <cmath>

namespace alto {

namespace {
// Frekuensi tone stack dan kabinet (Hz).
constexpr float kInputHighPass = 90.0f;
constexpr float kBassFrequency = 120.0f;
constexpr float kMidFrequency = 700.0f;
constexpr float kTrebleFrequency = 3200.0f;
constexpr float kPresenceFrequency = 4500.0f;
constexpr float kCabinetFrequency = 4800.0f;
constexpr float kShelfQ = 0.707f;
constexpr float kMidQ = 0.9f;

inline float dbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }

/** Penguatan tambahan (dB) sebelum saturasi untuk tiap voicing. */
inline float driveDbFor(float voicing) {
    // Clean / Crunch / Lead. Nilai dipilih agar preamp mulai terjenuh bertahap
    // (Crunch sedang, Lead penuh) tanpa langsung menjadi derau keras.
    static const float table[3] = {0.0f, 10.0f, 16.0f};
    const float v = voicing < 0.0f ? 0.0f : (voicing > 2.0f ? 2.0f : voicing);
    const int i = static_cast<int>(v);
    const float t = v - static_cast<float>(i);
    return table[i] + (table[i + 1 < 3 ? i + 1 : 2] - table[i]) * t;
}

/** Trim output (dB) agar keras tiap voicing seimbang. */
inline float outputTrimDbFor(float voicing) {
    static const float table[3] = {0.0f, -3.0f, -6.0f};
    const float v = voicing < 0.0f ? 0.0f : (voicing > 2.0f ? 2.0f : voicing);
    const int i = static_cast<int>(v);
    const float t = v - static_cast<float>(i);
    return table[i] + (table[i + 1 < 3 ? i + 1 : 2] - table[i]) * t;
}
}  // namespace

AmpModel::AmpModel() : Effect(7, "Amp") {
    setParam(0, 1.0f);    // voicing (Crunch)
    setParam(1, 8.0f);    // gain (dB)
    setParam(2, 2.0f);    // bass
    setParam(3, 0.0f);    // mid
    setParam(4, 2.0f);    // treble
    setParam(5, 0.0f);    // presence
    setParam(6, 0.0f);    // master (dB)
}

void AmpModel::prepare(double sampleRate, int32_t channels) {
    Effect::prepare(sampleRate, channels);
    const float sr = static_cast<float>(sampleRate);
    voicing_.configure(param(0), 30.0f, sampleRate);
    gain_.configure(param(1), 30.0f, sampleRate);
    bass_.configure(param(2), 30.0f, sampleRate);
    mid_.configure(param(3), 30.0f, sampleRate);
    treble_.configure(param(4), 30.0f, sampleRate);
    presence_.configure(param(5), 30.0f, sampleRate);
    master_.configure(param(6), 30.0f, sampleRate);

    for (int32_t c = 0; c < kMaxChannels; ++c) {
        inputHighPass_[c].setHighPass(sr, kInputHighPass, kShelfQ);
        cabinet_[c].setLowPass(sr, kCabinetFrequency, kShelfQ);
    }
    updateToneCoefficients(bass_.current(), mid_.current(), treble_.current(), presence_.current());
    saturator_.reset();
    oversampler_.prepare(sampleRate, channels);
    for (int32_t c = 0; c < kMaxChannels; ++c) {
        dcBlocker_[c].reset();
        inputHighPass_[c].reset();
        cabinet_[c].reset();
    }
}

void AmpModel::reset() {
    saturator_.reset();
    oversampler_.reset();
    for (int32_t c = 0; c < kMaxChannels; ++c) {
        dcBlocker_[c].reset();
        inputHighPass_[c].reset();
        bassFilter_[c].reset();
        midFilter_[c].reset();
        trebleFilter_[c].reset();
        presenceFilter_[c].reset();
        cabinet_[c].reset();
    }
}

void AmpModel::onParamChanged(int index, float value) {
    switch (index) {
        case 0: voicing_.setTarget(value); break;
        case 1: gain_.setTarget(value); break;
        case 2: bass_.setTarget(value); break;
        case 3: mid_.setTarget(value); break;
        case 4: treble_.setTarget(value); break;
        case 5: presence_.setTarget(value); break;
        case 6: master_.setTarget(value); break;
        default: break;
    }
}

void AmpModel::updateToneCoefficients(float bassDb, float midDb, float trebleDb,
                                      float presenceDb) {
    const float sr = static_cast<float>(sampleRate_);
    for (int32_t c = 0; c < kMaxChannels; ++c) {
        bassFilter_[c].setLowShelf(sr, kBassFrequency, kShelfQ, bassDb);
        midFilter_[c].setPeaking(sr, kMidFrequency, kMidQ, midDb);
        trebleFilter_[c].setHighShelf(sr, kTrebleFrequency, kShelfQ, trebleDb);
        presenceFilter_[c].setHighShelf(sr, kPresenceFrequency, kShelfQ, presenceDb);
    }
}

void AmpModel::process(float* buffer, int32_t frames, int32_t channels) {
    for (int32_t frame = 0; frame < frames; ++frame) {
        const float voicing = voicing_.next();
        const float gainDb = gain_.next();
        const float bassDb = bass_.next();
        const float midDb = mid_.next();
        const float trebleDb = treble_.next();
        const float presenceDb = presence_.next();
        const float masterDb = master_.next();

        if ((frame & 31) == 0) {
            updateToneCoefficients(bassDb, midDb, trebleDb, presenceDb);
        }

        const float preGain = dbToLinear(gainDb + driveDbFor(voicing));
        const float postGain = dbToLinear(masterDb + outputTrimDbFor(voicing));

        for (int32_t c = 0; c < channels; ++c) {
            float sample = buffer[frame * channels + c];
            sample = inputHighPass_[c].process(sample);
            sample *= preGain;

            // Saturasi preamp pada laju 2x untuk menekan aliasing (anti "pecah").
            float a = 0.0f;
            float b = 0.0f;
            oversampler_.up(sample, c, a, b);
            a = saturator_.processTanh(a, c);
            b = saturator_.processTanh(b, c);
            sample = oversampler_.down(a, b, c);

            sample = bassFilter_[c].process(sample);
            sample = midFilter_[c].process(sample);
            sample = trebleFilter_[c].process(sample);
            sample = presenceFilter_[c].process(sample);
            sample = cabinet_[c].process(sample);
            sample = dcBlocker_[c].process(sample);
            buffer[frame * channels + c] = sample * postGain;
        }
    }
}

}  // namespace alto
