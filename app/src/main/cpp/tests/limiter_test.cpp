// Uji limiter puncak lookahead (BAB 15).
//
// Tujuan: memastikan limiter (a) transparan di bawah batas, (b) menahan puncak
// tanpa melewati batas, dan (c) JAUH lebih halus daripada potong keras — inilah
// yang menghilangkan "pecah"/crackle saat level output tinggi.
#include "Limiter.h"
#include <cmath>
#include <cstdio>
#include <vector>

using namespace alto;

static float goertzel(const std::vector<float>& x, int start, int count, double freq, double sr) {
    const double w = 2.0 * M_PI * freq / sr;
    const double cw = std::cos(w);
    const double coeff = 2.0 * cw;
    double s0 = 0.0, s1 = 0.0, s2 = 0.0;
    for (int i = 0; i < count; ++i) {
        s0 = x[static_cast<size_t>(start + i)] + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    const double real = s1 - s2 * cw;
    const double imag = s2 * std::sin(w);
    return static_cast<float>(std::sqrt(real * real + imag * imag) / (count / 2.0));
}

int main() {
    const double sr = 48000.0;
    const int frames = 48000;  // 1 detik
    const double freq = 1000.0;
    int pass = 0, total = 0;

    std::vector<float> sine(static_cast<size_t>(frames));
    for (int i = 0; i < frames; ++i) {
        sine[static_cast<size_t>(i)] = static_cast<float>(2.0 * std::sin(2.0 * M_PI * freq * i / sr));
    }

    // 1) Transparan di bawah batas: sinyal 0.5 tidak berubah (setelah tunda).
    {
        std::vector<float> buf(static_cast<size_t>(frames));
        for (int i = 0; i < frames; ++i) {
            buf[static_cast<size_t>(i)] =
                static_cast<float>(0.5 * std::sin(2.0 * M_PI * freq * i / sr));
        }
        const std::vector<float> original = buf;
        Limiter limiter;
        limiter.prepare(sr, 1, 0.0f);
        limiter.process(buf.data(), frames, 1);
        // Limiter menunda sinyal sepanjang lookahead (~1,5 ms = 72 sampel pada
        // 48 kHz); bandingkan setelah menyelaraskan tunda tersebut.
        const int lookahead = static_cast<int>(1.5 * 0.001 * sr);
        float maxDiff = 0.0f;
        for (int i = lookahead + 200; i < frames; ++i) {
            maxDiff = std::max(
                maxDiff,
                std::fabs(buf[static_cast<size_t>(i)] - original[static_cast<size_t>(i - lookahead)]));
        }
        const bool ok = maxDiff < 1e-3f;
        printf("Transparan (0.5): selisih maks=%.2e -> %s\n", maxDiff, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 2) Menahan puncak: sinyal 2.0 dibatasi ke ceiling 1.0 tanpa melewatinya.
    {
        std::vector<float> buf = sine;
        Limiter limiter;
        limiter.prepare(sr, 1, 0.0f);
        limiter.process(buf.data(), frames, 1);
        float peak = 0.0f;
        for (int i = 100; i < frames; ++i) {
            peak = std::max(peak, std::fabs(buf[static_cast<size_t>(i)]));
        }
        const bool ok = peak <= 1.0f + 1e-4f && peak >= 0.98f;
        printf("Batasi (2.0 -> ceiling 1.0): puncak=%.4f -> %s\n", peak, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 3) Kehalusan: distorsi harmonisa limiter jauh lebih rendah daripada
    //    potong keras. Ini bukti kuantitatif "tidak pecah".
    {
        std::vector<float> limited = sine;
        Limiter limiter;
        limiter.prepare(sr, 1, 0.0f);
        limiter.process(limited.data(), frames, 1);

        std::vector<float> hardClip = sine;
        for (int i = 0; i < frames; ++i) {
            float& s = hardClip[static_cast<size_t>(i)];
            if (s > 1.0f) s = 1.0f;
            else if (s < -1.0f) s = -1.0f;
        }

        // Ukur pada segmen mantap (lewati awal) untuk menghindari transien.
        const int start = 4800;      // 0,1 detik
        const int count = 24000;     // 0,5 detik
        const float f1 = goertzel(limited, start, count, freq, sr);
        const float h3l = goertzel(limited, start, count, 3.0 * freq, sr);
        const float h5l = goertzel(limited, start, count, 5.0 * freq, sr);
        const float thdLimiter = std::sqrt(h3l * h3l + h5l * h5l) / (f1 + 1e-9f);

        const float f1c = goertzel(hardClip, start, count, freq, sr);
        const float h3c = goertzel(hardClip, start, count, 3.0 * freq, sr);
        const float h5c = goertzel(hardClip, start, count, 5.0 * freq, sr);
        const float thdHard = std::sqrt(h3c * h3c + h5c * h5c) / (f1c + 1e-9f);

        const bool ok = thdLimiter < thdHard * 0.5f;
        printf("Kehalusan: THD limiter=%.3f vs potong keras=%.3f -> %s\n",
               thdLimiter, thdHard, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    printf("Limiter: %d/%d lolos\n", pass, total);
    return (pass == total) ? 0 : 1;
}
