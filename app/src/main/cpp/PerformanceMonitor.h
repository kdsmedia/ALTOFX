#ifndef ALTOGUITARFX_PERFORMANCEMONITOR_H
#define ALTOGUITARFX_PERFORMANCEMONITOR_H

#include <atomic>
#include <cstdint>

namespace alto {

/**
 * Pemantau performa audio callback (BAB 16).
 *
 * Mencatat lama eksekusi tiap callback lalu menghitung beban CPU relatif
 * terhadap anggaran waktu satu blok (frames / sampleRate). Nilai disimpan
 * atomik sehingga aman dibaca dari thread UI.
 *
 * Beban mendekati 1.0 berarti callback hampir kehabisan waktu -> risiko
 * drop-out/crackle. Statistik puncak membantu menemukan lonjakan sesekali.
 */
class PerformanceMonitor {
public:
    void prepare(double sampleRate) {
        sampleRate_.store(sampleRate > 0.0 ? sampleRate : 48000.0);
        reset();
    }

    void reset() {
        callbacks_.store(0);
        totalMicros_.store(0.0);
        peakMicros_.store(0.0);
        totalLoad_.store(0.0);
        peakLoad_.store(0.0);
    }

    /** Mencatat satu callback. [durationMicros] lama eksekusi, [frames] jumlah frame. */
    void record(double durationMicros, int32_t frames) {
        const double rate = sampleRate_.load();
        const double budgetMicros = (frames > 0) ? (frames / rate * 1.0e6) : 0.0;
        const double load = (budgetMicros > 0.0) ? (durationMicros / budgetMicros) : 0.0;

        callbacks_.fetch_add(1);
        totalMicros_.store(totalMicros_.load() + durationMicros);
        totalLoad_.store(totalLoad_.load() + load);
        if (durationMicros > peakMicros_.load()) {
            peakMicros_.store(durationMicros);
        }
        if (load > peakLoad_.load()) {
            peakLoad_.store(load);
        }
    }

    int64_t callbacks() const { return callbacks_.load(); }

    double averageMicros() const {
        const int64_t n = callbacks_.load();
        return (n > 0) ? (totalMicros_.load() / static_cast<double>(n)) : 0.0;
    }

    double peakMicros() const { return peakMicros_.load(); }

    /** Beban rata-rata 0..1 relatif terhadap anggaran waktu blok. */
    float averageLoad() const {
        const int64_t n = callbacks_.load();
        return (n > 0) ? static_cast<float>(totalLoad_.load() / static_cast<double>(n)) : 0.0f;
    }

    /** Beban puncak 0..1. */
    float peakLoad() const { return static_cast<float>(peakLoad_.load()); }

private:
    std::atomic<double> sampleRate_{48000.0};
    std::atomic<int64_t> callbacks_{0};
    std::atomic<double> totalMicros_{0.0};
    std::atomic<double> peakMicros_{0.0};
    std::atomic<double> totalLoad_{0.0};
    std::atomic<double> peakLoad_{0.0};
};

}  // namespace alto

#endif  // ALTOGUITARFX_PERFORMANCEMONITOR_H
