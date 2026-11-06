#ifndef ALTOGUITARFX_EFFECTS_H
#define ALTOGUITARFX_EFFECTS_H

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

#include "Effect.h"

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
    Equalizer = 5,
    Chorus = 6,
    Flanger = 7,
    Phaser = 8,
    Tremolo = 9,
    Delay = 10,
    Reverb = 11,
    Count = 12
};

/**
 * Pengelola rangkaian efek (BAB 7+).
 *
 * Efek yang belum diimplementasikan bernilai null dan dilewati. Semua metode
 * pengaturan aman dipanggil dari thread UI; [process] dipanggil dari jalur
 * real-time dan tidak mengalokasi memori.
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
    std::atomic<uint64_t> orderCode_{0};
    std::atomic<bool> globalBypass_{false};
};

}  // namespace alto

#endif  // ALTOGUITARFX_EFFECTS_H
