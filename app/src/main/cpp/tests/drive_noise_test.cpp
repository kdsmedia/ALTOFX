// Uji kebersihan efek saturasi (Overdrive/Distortion/Fuzz).
//
// Regresi utama: saat gitar diam, efek distorsi TIDAK boleh "mendengung" atau
// "berisik". Sebelumnya sisa dengung frekuensi rendah (50/150 Hz) lolos ke
// tahap saturasi lalu dikuatkan puluhan kali sehingga keluaran hampir sekeras
// saat bermain — noise gate tidak pernah menutup. Uji ini memastikan jalur
// nyata (HumFilter -> NoiseGate -> efek) tetap hampir senyap saat diam.
#include "Effects.h"
#include "HumFilter.h"
#include "NoiseGate.h"

#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

using namespace alto;

static double rms(const std::vector<float>& v, int ch, int c) {
    double s = 0.0;
    int n = 0;
    for (size_t i = c; i < v.size(); i += ch) { s += v[i] * v[i]; ++n; }
    return std::sqrt(s / n);
}

static float peak(const std::vector<float>& v) {
    float p = 0.0f;
    for (float s : v) p = std::max(p, std::fabs(s));
    return p;
}

int main() {
    const double sr = 48000.0;
    const int ch = 2;
    const int frames = static_cast<int>(sr * 2.0);
    int pass = 0, total = 0;

    std::mt19937 rng(7);
    std::normal_distribution<float> noise(0.0f, 1.0f);
    // Gitar 0..0.5 s, lalu diam; dengung 50/100/150 Hz + derau tetap ada.
    std::vector<float> base(static_cast<size_t>(frames) * ch);
    for (int i = 0; i < frames; ++i) {
        const double t = i / sr;
        double v = 0.0006 * noise(rng);
        v += 0.02 * std::sin(2 * M_PI * 50 * t);
        v += 0.012 * std::sin(2 * M_PI * 100 * t);
        v += 0.012 * std::sin(2 * M_PI * 150 * t);
        if (i < 24000) {
            v += 0.08 * std::sin(2 * M_PI * 220 * t) + 0.05 * std::sin(2 * M_PI * 440 * t);
        }
        base[static_cast<size_t>(i) * ch] = static_cast<float>(v);
        base[static_cast<size_t>(i) * ch + 1] = static_cast<float>(v);
    }

    const char* names[3] = {"Overdrive", "Distortion", "Fuzz"};
    const int ids[3] = {2, 3, 4};

    for (int e = 0; e < 3; ++e) {
        HumFilter hum; hum.prepare(sr, ch); hum.setSampleRate(sr); hum.setHumFrequency(50.0f);
        NoiseGate gate; gate.prepare(sr, ch);
        gate.setParam(0, -55.0f); gate.setParam(1, 150.0f); gate.setParam(2, 5.0f);
        gate.setEnabled(true);
        EffectChain chain; chain.prepare(sr, ch);
        chain.setEnabled(static_cast<EffectType>(ids[e]), true);

        auto buf = base;
        hum.process(buf.data(), frames, ch);
        gate.process(buf.data(), frames, ch);
        chain.process(buf.data(), frames, ch);

        // Wilayah diam: 1 detik terakhir.
        std::vector<float> tail(buf.begin() + static_cast<size_t>(frames - 48000) * ch, buf.end());
        const double idleRms = rms(tail, ch, 0);
        // Wilayah bermain: 0,2..0,4 s (sudah tunak).
        std::vector<float> play(buf.begin() + static_cast<size_t>(9600) * ch,
                                buf.begin() + static_cast<size_t>(19200) * ch);
        const double playRms = rms(play, ch, 0);

        // Saat diam harus hampir senyap (< -40 dBFS RMS) dan jauh di bawah
        // level saat bermain (rasio > 30x). Inilah yang menghilangkan dengung.
        const bool quiet = idleRms < 0.01;
        const bool contrast = playRms > idleRms * 30.0;
        const bool ok = quiet && contrast;
        printf("%-11s diam_rms=%.5f main_rms=%.4f rasio=%.0fx -> %s\n",
               names[e], idleRms, playRms, playRms / (idleRms + 1e-9), ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    printf("\nKebersihan saturasi: %d/%d lolos\n", pass, total);
    return pass == total ? 0 : 1;
}
