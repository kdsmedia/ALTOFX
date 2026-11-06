#ifndef ALTOGUITARFX_GAINSTAGE_H
#define ALTOGUITARFX_GAINSTAGE_H

#include <atomic>
#include <cmath>
#include <cstdint>

namespace alto {

/**
 * Tahap gain dengan peredaman perubahan (BAB 15).
 *
 * Dipakai untuk trim input dan level output master. Nilai target diatur dari
 * thread UI lewat atomik; audio callback meratakan perubahan secara per-sampel
 * dengan filter satu kutub sehingga tidak terdengar klik/pop. Tidak ada
 * alokasi memori di jalur audio.
 */
class GainStage {
public:
    /** Konversi desibel ke faktor linier. */
    static float dbToLinear(float db) {
        return std::pow(10.0f, db / 20.0f);
    }

    /** Konversi faktor linier ke desibel. */
    static float linearToDb(float linear) {
        return (linear > 1e-6f) ? (20.0f * std::log10(linear)) : -120.0f;
    }

    /** Menyiapkan peredaman; [timeConstantMs] mengatur kecepatan perataan. */
    void prepare(double sampleRate, float timeConstantMs = 20.0f) {
        if (sampleRate > 0.0 && timeConstantMs > 0.0f) {
            smoothingCoeff_ = 1.0f - std::exp(
                -1.0f / (static_cast<float>(sampleRate) * timeConstantMs * 0.001f));
        } else {
            smoothingCoeff_ = 1.0f;
        }
    }

    /** Mengatur target gain dalam desibel. */
    void setTargetDb(float db) {
        linearTarget_.store(dbToLinear(db));
        applyEffectiveTarget();
    }

    /** Mengatur target gain sebagai faktor linier. */
    void setTargetLinear(float linear) {
        linearTarget_.store(linear);
        applyEffectiveTarget();
    }

    /**
     * Mute: bila aktif, target efektif menjadi 0 sehingga sinyal meredup
     * dengan perataan (bukan potong mendadak) dan tidak menimbulkan klik.
     */
    void setMuted(bool muted) {
        muted_.store(muted);
        applyEffectiveTarget();
    }

    bool muted() const { return muted_.load(); }

    float targetDb() const { return linearToDb(linearTarget_.load()); }

    float targetLinear() const { return linearTarget_.load(); }

    /** Nilai gain yang sedang dipakai (hasil perataan terakhir). */
    float currentLinear() const { return current_; }

    /** Menerapkan gain ke buffer dengan perataan per-sampel. */
    void process(float* buffer, int32_t frames, int32_t channels) {
        const float target = target_.load();
        for (int32_t frame = 0; frame < frames; ++frame) {
            current_ += smoothingCoeff_ * (target - current_);
            for (int32_t channel = 0; channel < channels; ++channel) {
                buffer[frame * channels + channel] *= current_;
            }
        }
    }

    /** Menyetel gain langsung tanpa perataan (untuk inisialisasi). */
    void snapToTarget() { current_ = target_.load(); }

private:
    void applyEffectiveTarget() {
        target_.store(muted_.load() ? 0.0f : linearTarget_.load());
    }

    std::atomic<float> linearTarget_{1.0f};
    std::atomic<bool> muted_{false};
    std::atomic<float> target_{1.0f};
    float current_ = 1.0f;
    float smoothingCoeff_ = 1.0f;
};

}  // namespace alto

#endif  // ALTOGUITARFX_GAINSTAGE_H
