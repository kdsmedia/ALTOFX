#ifndef ALTOGUITARFX_PITCHDETECTOR_H
#define ALTOGUITARFX_PITCHDETECTOR_H

#include <atomic>
#include <cstdint>

namespace alto {

/**
 * Detektor pitch (BAB 14) memakai algoritma YIN.
 *
 * Kelas ini menerima sampel input gitar dari audio callback, menampungnya
 * sampai satu jendela penuh, lalu menghitung periode dasar sinyal. Hasilnya
 * (frekuensi, nada MIDI, selisih cents, dan keyakinan) disimpan sebagai
 * atomik sehingga aman dibaca dari thread UI.
 *
 * Tidak ada alokasi memori di jalur audio: seluruh buffer berukuran tetap.
 */
class PitchDetector {
public:
    /** Panjang jendela analisis dalam sampel (cukup untuk nada rendah gitar). */
    static constexpr int32_t kWindowSize = 2048;
    /** Ambang batas YIN; nilai lebih kecil dari ini dianggap periode dasar. */
    static constexpr float kThreshold = 0.15f;
    /** Rentang nada gitar yang didukung (Hz). */
    static constexpr float kMinFrequency = 60.0f;
    static constexpr float kMaxFrequency = 1500.0f;

    PitchDetector() = default;

    /** Menyiapkan detector; boleh dipanggil dari thread UI. */
    void prepare(double sampleRate);

    /** Mengosongkan buffer dan hasil. */
    void reset();

    /** Mengaktifkan/mematikan analisis untuk menghemat CPU saat tidak dipakai. */
    void setActive(bool active) { active_.store(active); }

    bool isActive() const { return active_.load(); }

    /**
     * Menambahkan blok input ke buffer analisis. Dipanggil dari audio callback;
     * hanya membaca kanal pertama. Bila jendela penuh, analisis dijalankan.
     */
    void process(const float* input, int32_t frames, int32_t channels);

    /** Frekuensi dasar terakhir (Hz), 0 bila tidak ada nada. */
    float frequency() const { return frequency_.load(); }

    /** Nada MIDI terakhir (mis. 69 = A4), -1 bila tidak ada nada. */
    float midiNote() const { return midiNote_.load(); }

    /** Selisih dari nada terdekat dalam cents, 0 bila tidak ada nada. */
    float cents() const { return cents_.load(); }

    /** Keyakinan deteksi 0..1 (1 - nilai YIN minimum). */
    float confidence() const { return confidence_.load(); }

    /** Mengatur referensi A4 (Hz), default 440. */
    void setReferenceHz(float referenceHz) { referenceHz_.store(referenceHz); }

    float referenceHz() const { return referenceHz_.load(); }

private:
    /** Menghitung frekuensi dasar dari isi buffer; 0 bila tidak ada nada. */
    float detect(float& confidenceOut);

    std::atomic<bool> active_{false};
    std::atomic<float> frequency_{0.0f};
    std::atomic<float> midiNote_{-1.0f};
    std::atomic<float> cents_{0.0f};
    std::atomic<float> confidence_{0.0f};
    std::atomic<float> referenceHz_{440.0f};

    double sampleRate_ = 48000.0;
    int32_t fill_ = 0;
    float buffer_[kWindowSize]{};
    float difference_[kWindowSize / 2 + 1]{};
    float cumulative_[kWindowSize / 2 + 1]{};
};

}  // namespace alto

#endif  // ALTOGUITARFX_PITCHDETECTOR_H
