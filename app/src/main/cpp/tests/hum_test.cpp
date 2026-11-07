#include "HumFilter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace alto;

// Mengukur RMS sinyal pada satu frekuensi setelah filter mencapai keadaan
// tunak. Sinyal diumpankan sebagai blok berurutan (fase berlanjut) dan RMS
// diukur pada blok terakhir; filter tidak di-reset agar transien hilang.
static double rmsAt(double sr, double freq, HumFilter& filter, int block, int blocks) {
    std::vector<float> buf(static_cast<size_t>(block));
    double sum = 0.0;
    long long phase = 0;
    for (int b = 0; b < blocks; ++b) {
        for (int i = 0; i < block; ++i) {
            buf[static_cast<size_t>(i)] =
                static_cast<float>(std::sin(2.0 * M_PI * freq * (phase + i) / sr));
        }
        phase += block;
        filter.process(buf.data(), block, 1);
    }
    for (float s : buf) sum += static_cast<double>(s) * s;
    return std::sqrt(sum / block);
}

int main() {
    const double sr = 48000.0;
    const int block = 4800;
    const int blocks = 40;  // ~4 detik agar notch sempit mencapai tunak
    int pass = 0, total = 0;

    // 1) Dengung 50 Hz (dan harmonisa 100/150 Hz) diredam, nada gitar 220 Hz lolos.
    {
        HumFilter filter;
        filter.prepare(sr, 1);
        filter.setHumFrequency(50.0f);

        const double hum50 = rmsAt(sr, 50.0, filter, block, blocks);
        filter.reset();
        const double hum100 = rmsAt(sr, 100.0, filter, block, blocks);
        filter.reset();
        const double hum150 = rmsAt(sr, 150.0, filter, block, blocks);
        filter.reset();
        const double guitar = rmsAt(sr, 220.0, filter, block, blocks);

        // RMS sinus murni = 0.707; notch harus menekan jauh di bawah itu,
        // sementara nada gitar hampir tidak berubah. Harmonisa ketiga (150 Hz)
        // penting karena berada di rentang rendah gitar dan menguatkan dengung
        // pada efek distorsi bila tidak dibuang.
        const bool ok = hum50 < 0.15 && hum100 < 0.35 && hum150 < 0.15 && guitar > 0.6;
        printf("HumFilter: 50Hz=%.3f 100Hz=%.3f 150Hz=%.3f 220Hz=%.3f -> %s\n",
               hum50, hum100, hum150, guitar, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 2) Mode 60 Hz meredam dengung 60 Hz, nada di luar notch lolos.
    {
        HumFilter filter;
        filter.prepare(sr, 1);
        filter.setHumFrequency(60.0f);
        const double hum60 = rmsAt(sr, 60.0, filter, block, blocks);
        filter.reset();
        const double tone300 = rmsAt(sr, 300.0, filter, block, blocks);
        const bool ok = hum60 < 0.15 && tone300 > 0.6;
        printf("HumFilter 60Hz: 60Hz=%.3f 300Hz=%.3f -> %s\n",
               hum60, tone300, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 3) Offset DC dibuang (grounding dapat menambah tegangan DC).
    {
        HumFilter filter;
        filter.prepare(sr, 1);
        std::vector<float> buf(block, 0.3f);
        for (int b = 0; b < 20; ++b) filter.process(buf.data(), block, 1);
        float mean = 0.0f;
        for (float s : buf) mean += s;
        mean /= static_cast<float>(block);
        const bool ok = std::fabs(mean) < 1e-3f;
        printf("HumFilter DC: rata-rata=%.5f -> %s\n", mean, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 4) Stabil pada 96 kHz.
    {
        HumFilter filter;
        filter.prepare(96000.0, 2);
        filter.setHumFrequency(50.0f);
        std::vector<float> buf(static_cast<size_t>(block) * 2);
        for (int i = 0; i < block; ++i) {
            const float v = static_cast<float>(0.5 * std::sin(2.0 * M_PI * 50.0 * i / 96000.0));
            buf[static_cast<size_t>(i) * 2] = v;
            buf[static_cast<size_t>(i) * 2 + 1] = v;
        }
        for (int b = 0; b < 20; ++b) filter.process(buf.data(), block, 2);
        bool finite = true;
        for (float s : buf) {
            if (!std::isfinite(s) || std::fabs(s) > 4.0f) { finite = false; break; }
        }
        printf("HumFilter @96kHz stereo: stabil -> %s\n", finite ? "OK" : "GAGAL");
        pass += finite; total++;
    }

    printf("\nPeredam dengung: %d/%d lolos\n", pass, total);
    return pass == total ? 0 : 1;
}
