#ifndef ALTOGUITARFX_EFFECT_H
#define ALTOGUITARFX_EFFECT_H

#include <atomic>
#include <cmath>
#include <cstdint>

namespace alto {

/**
 * Nilai parameter yang dihaluskan (smoothed) agar perubahan dari UI tidak
 * menimbulkan klik/pop. Semua operasi bersifat real-time (tanpa alokasi).
 */
class SmoothedParam {
public:
    void configure(float initial, float timeMs, double sampleRate) {
        current_ = initial;
        target_.store(initial);
        const float samples = timeMs * 0.001f * static_cast<float>(sampleRate);
        coefficient_ = (samples > 1.0f) ? (1.0f - std::exp(-1.0f / samples)) : 1.0f;
    }

    void setTarget(float value) { target_.store(value); }

    float target() const { return target_.load(); }

    /** Nilai terhalus untuk sampel berikutnya. */
    float next() {
        const float targetValue = target_.load();
        current_ += (targetValue - current_) * coefficient_;
        if (std::fabs(targetValue - current_) < 1e-6f) {
            current_ = targetValue;
        }
        return current_;
    }

    float current() const { return current_; }

private:
    std::atomic<float> target_{0.0f};
    float current_ = 0.0f;
    float coefficient_ = 1.0f;
};

/**
 * Dasar semua efek DSP (BAB 7+).
 *
 * Setiap efek menerima penyangga audio antarleaved float dan memprosesnya
 * di tempat (in-place). Parameter dibaca dari larik atomik milik efek
 * (lihat [params]); nilai dibaca sekali per blok lalu dihaluskan per sampel.
 */
class Effect {
public:
    Effect(int paramCount, const char* name) : paramCount_(paramCount), name_(name) {}

    virtual ~Effect() = default;

    /** Dipanggil sebelum audio berjalan; boleh mengalokasikan memori. */
    virtual void prepare(double sampleRate, int32_t channels) {
        sampleRate_ = sampleRate;
        channels_ = channels;
        reset();
    }

    /** Menyetel ulang keadaan internal (mis. saat start/stop). */
    virtual void reset() {}

    /** Memproses blok audio di tempat. Dipanggil dari jalur real-time. */
    virtual void process(float* buffer, int32_t frames, int32_t channels) = 0;

    void setEnabled(bool enabled) { enabled_.store(enabled); }

    bool isEnabled() const { return enabled_.load(); }

    void setParam(int index, float value) {
        if (index >= 0 && index < paramCount_) {
            params_[index].store(value);
            onParamChanged(index, value);
        }
    }

    float param(int index) const {
        if (index >= 0 && index < paramCount_) {
            return params_[index].load();
        }
        return 0.0f;
    }

    int paramCount() const { return paramCount_; }

    const char* name() const { return name_; }

protected:
    /**
     * Dipanggil setiap parameter berubah dari UI. Efek menyalurkan nilai ke
     * SmoothedParam-nya agar perubahan diterapkan tanpa klik.
     */
    virtual void onParamChanged(int /*index*/, float /*value*/) {}

    double sampleRate_ = 48000.0;
    int32_t channels_ = 1;
    int paramCount_;
    const char* name_;
    std::atomic<bool> enabled_{false};
    // Maksimum 8 parameter per efek; cukup untuk semua efek di rancangan.
    std::atomic<float> params_[8]{};
};

}  // namespace alto

#endif  // ALTOGUITARFX_EFFECT_H
