#ifndef ALTOGUITARFX_EFFECTS_H
#define ALTOGUITARFX_EFFECTS_H

#include <array>
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
 */
class EffectChain {
public:
    EffectChain();

    /** Menyiapkan seluruh efek; boleh mengalokasikan memori. */
    void prepare(double sampleRate, int32_t channels);

    /** Memproses blok audio melalui seluruh efek aktif, berurutan. */
    void process(float* buffer, int32_t frames, int32_t channels);

    void setEnabled(EffectType type, bool enabled);
    bool isEnabled(EffectType type) const;

    void setParam(EffectType type, int index, float value);
    float param(EffectType type, int index) const;
    int paramCount(EffectType type) const;
    const char* name(EffectType type) const;

private:
    Effect* effect(EffectType type) const;

    std::array<std::unique_ptr<Effect>, static_cast<size_t>(EffectType::Count)> effects_;
};

}  // namespace alto

#endif  // ALTOGUITARFX_EFFECTS_H
