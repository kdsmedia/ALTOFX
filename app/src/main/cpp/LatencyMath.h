#ifndef ALTOGUITARFX_LATENCYMATH_H
#define ALTOGUITARFX_LATENCYMATH_H

#include <cstdint>

namespace alto {

/**
 * Perhitungan latensi round-trip dari timestamp perangkat (BAB 16).
 *
 * Angka berasal dari posisi frame nyata yang dilaporkan Oboe:
 *   - framesWritten / outputPresentedPos: berapa frame keluaran yang sudah
 *     ditulis tetapi belum diputar (jeda ke depan).
 *   - framesRead / inputCapturedPos: berapa frame masukan yang sudah dibaca
 *     tetapi baru saja ditangkap (jeda ke belakang).
 * Keduanya dijumlahkan lalu dibagi sample rate, sehingga hasilnya adalah
 * perkiraan latensi perangkat-ke-perangkat yang diukur, bukan angka teoretis.
 *
 * Mengembalikan nilai milidetik, atau -1 bila data tidak valid (timestamp
 * belum didukung perangkat atau nilainya tidak masuk akal).
 */
inline double roundTripLatencyMs(double sampleRate,
                                 int64_t framesWritten,
                                 int64_t outputPresentedPos,
                                 int64_t framesRead,
                                 int64_t inputCapturedPos) {
    if (sampleRate <= 0.0) {
        return -1.0;
    }
    const double outputAhead = static_cast<double>(framesWritten - outputPresentedPos);
    const double inputBehind = static_cast<double>(framesRead - inputCapturedPos);
    if (outputAhead < 0.0 || inputBehind < 0.0) {
        return -1.0;
    }
    const double ms = (outputAhead + inputBehind) / sampleRate * 1000.0;
    // Di atas 1 detik hampir pasti data timestamp tidak dapat dipercaya.
    if (ms < 0.0 || ms > 1000.0) {
        return -1.0;
    }
    return ms;
}

}  // namespace alto

#endif  // ALTOGUITARFX_LATENCYMATH_H
