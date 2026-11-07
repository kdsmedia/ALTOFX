#ifndef ALTOGUITARFX_EFFECTS_H
#define ALTOGUITARFX_EFFECTS_H

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

#include "Effect.h"
#include "NoiseCleanup.h"

namespace alto {


/**
 * Urutan tetap efek pada versi pertama (lihat PDF bagian 3):
 * Noise Gate -> Compressor -> Drive/Distortion/Fuzz -> EQ -> Modulation ->
 * Delay -> Reverb.
 *
 * Nilai enum dipakai sebagai indeks stabil dari sisi UI/JNI.
 */
enum class EffectType : int {
    NoiseGate = 0,
    Compressor = 1,
    Overdrive = 2,
    Distortion = 3,
    Fuzz = 4,
    Amp = 5,
    Equalizer = 6,
    Chorus = 7,
    Flanger = 8,
    Phaser = 9,
    Tremolo = 10,
    Delay = 11,
    Reverb = 12,
    AutoWah = 13,
    Count = 14
};

/**
 * Pengelola rangkaian efek (BAB 7+).
 *
 * Seluruh 13 efek sudah terpasang; pengecekan null tetap dipertahankan sebagai
 * pertahanan bila suatu slot belum terisi. Semua metode pengaturan aman
 * dipanggil dari thread UI; [process] dipanggil dari jalur real-time dan tidak
 * mengalokasi memori.
 *
 * Urutan pemrosesan dapat diubah dari UI (BAB 12). Urutan disimpan sebagai
 * kode 4-bit per slot di dalam satu bilangan 64-bit atomik, sehingga audio
 * callback dapat membacanya tanpa kunci dan tanpa menyalin larik besar.
 */
class EffectChain {
public:
    static constexpr int kEffectCount = static_cast<int>(EffectType::Count);

    EffectChain();

    /** Menyiapkan seluruh efek; boleh mengalokasikan memori. */
    void prepare(double sampleRate, int32_t channels);

    /** Memproses blok audio melalui seluruh efek aktif, sesuai urutan. */
    void process(float* buffer, int32_t frames, int32_t channels);

    void setEnabled(EffectType type, bool enabled);
    bool isEnabled(EffectType type) const;

    /**
     * Tombol "Noise" per efek: mengaktifkan/mematikan pembersih bising pada
     * jalur efek tersebut. Berlaku untuk semua efek (juga yang belum aktif).
     */
    void setNoiseReduction(EffectType type, bool on);
    bool noiseReduction(EffectType type) const;

    void setParam(EffectType type, int index, float value);
    float param(EffectType type, int index) const;
    int paramCount(EffectType type) const;
    const char* name(EffectType type) const;

    /** Urutan pemrosesan saat ini. */
    std::array<EffectType, kEffectCount> order() const;

    /** Mengganti urutan pemrosesan; nilai tidak valid diabaikan. */
    void setOrder(const std::array<EffectType, kEffectCount>& order);

    /** Menggeser satu efek naik (delta<0) atau turun (delta>0) satu langkah. */
    void moveEffect(EffectType type, int delta);

    /** Mengembalikan urutan ke bawaan. */
    void resetOrder();

    /**
     * Menyetel ulang keadaan internal seluruh efek (blueprint bagian 7:
     * setiap efek punya proses reset). Dipanggil saat audio berhenti agar
     * ekor delay/reverb lama tidak berbunyi lagi setelah stream dimulai ulang.
     */
    void reset();

    /**
     * Bypass keseluruhan: bila aktif, seluruh efek dilewati dan sinyal
     * diteruskan apa adanya (dry). Dipakai untuk tombol bypass global.
     */
    void setGlobalBypass(bool bypass) { globalBypass_.store(bypass); }

    bool globalBypass() const { return globalBypass_.load(); }

private:
    Effect* effect(EffectType type) const;
    static std::array<EffectType, kEffectCount> decode(uint64_t code);
    static uint64_t encode(const std::array<EffectType, kEffectCount>& order);

    std::array<std::unique_ptr<Effect>, static_cast<size_t>(EffectType::Count)> effects_;
    // Pembersih bising per slot efek (tombol "Noise" pada popup tiap efek).
    std::array<NoiseCleanup, static_cast<size_t>(EffectType::Count)> noiseCleanup_;
    std::atomic<uint64_t> orderCode_{0};
    std::atomic<bool> globalBypass_{false};
};

}  // namespace alto

#endif  // ALTOGUITARFX_EFFECTS_H
