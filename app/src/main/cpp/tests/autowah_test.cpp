#include "AutoWah.h"
#include "Effects.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace alto;

// Membangkitkan sinyal gitar: petikan kuat lalu meluruh, lalu diam.
static std::vector<float> pluck(double sr, int n, double f0) {
    std::vector<float> out(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        const double t = i / sr;
        const double env = std::exp(-5.0 * t);
        out[static_cast<size_t>(i)] =
            static_cast<float>(0.6 * std::sin(2 * M_PI * f0 * t) * env);
    }
    return out;
}

// Spektrum pada frekuensi tertentu (untuk memeriksa pergeseran puncak).
static double mag(const std::vector<float>& x, double sr, double f) {
    double re = 0, im = 0;
    const int n = static_cast<int>(x.size());
    for (int i = 0; i < n; ++i) {
        re += x[static_cast<size_t>(i)] * std::cos(2 * M_PI * f * i / sr);
        im += x[static_cast<size_t>(i)] * std::sin(2 * M_PI * f * i / sr);
    }
    return std::sqrt(re * re + im * im) / n;
}

int main() {
    const double sr = 48000.0;
    const int ch = 1;
    int pass = 0, total = 0;

    // 1) AutoWah mengubah sinyal saat aktif dan identik saat bypass.
    {
        AutoWah fx;
        fx.prepare(sr, ch);
        fx.setParam(0, 0.8f); fx.setParam(1, 0.9f);
        fx.setParam(2, 6.0f); fx.setParam(3, 1.0f);
        fx.setEnabled(true);
        auto buf = pluck(sr, 24000, 110.0);
        const auto dry = buf;
        fx.process(buf.data(), 24000, ch);
        double diff = 0.0;
        for (size_t i = 0; i < buf.size(); ++i) {
            const double d = buf[i] - dry[i];
            diff += d * d;
        }
        const double rms = std::sqrt(diff / buf.size());
        const bool active = std::isfinite(rms) && rms > 1e-3;

        // Bypass dikelola oleh EffectChain: efek nonaktif = sinyal apa adanya.
        EffectChain chain;
        chain.prepare(sr, ch);
        for (int i = 0; i < EffectChain::kEffectCount; ++i) {
            chain.setEnabled(static_cast<EffectType>(i), false);
        }
        auto out = pluck(sr, 24000, 110.0);
        const auto expect = out;
        chain.process(out.data(), 24000, ch);
        bool identical = true;
        for (size_t i = 0; i < out.size(); ++i) {
            if (out[i] != expect[i]) { identical = false; break; }
        }
        const bool ok = active && identical;
        printf("AutoWah aktif ubah=%.4f bypass identik=%d -> %s\n",
               rms, identical, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 2) Karakter envelope: petikan kuat membuka filter lebih terang daripada
    //    nada yang meluruh (energi tinggi meningkat saat awal petikan).
    {
        AutoWah fx;
        fx.prepare(sr, ch);
        fx.setParam(0, 1.0f); fx.setParam(1, 1.0f);
        fx.setParam(2, 8.0f); fx.setParam(3, 1.0f);
        fx.setEnabled(true);
        auto buf = pluck(sr, 24000, 110.0);
        fx.process(buf.data(), 24000, ch);
        // Bandingkan energi sekitar puncak (awal) dengan ekor (meluruh).
        const int seg = 2048;
        std::vector<float> head(buf.begin(), buf.begin() + seg);
        std::vector<float> tail(buf.begin() + 18000, buf.begin() + 18000 + seg);
        const double headHigh = mag(head, sr, 1200.0) + mag(head, sr, 1800.0);
        const double tailHigh = mag(tail, sr, 1200.0) + mag(tail, sr, 1800.0);
        const bool ok = std::isfinite(headHigh) && std::isfinite(tailHigh) &&
                        headHigh > tailHigh;
        printf("AutoWah envelope: tinggi(awal)=%.5f tinggi(ekor)=%.5f -> %s\n",
               headHigh, tailHigh, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 3) Stabil dan berhingga pada 96 kHz stereo.
    {
        AutoWah fx;
        fx.prepare(96000.0, 2);
        fx.setParam(0, 0.9f); fx.setParam(1, 0.9f);
        fx.setParam(2, 9.0f); fx.setParam(3, 1.0f);
        fx.setEnabled(true);
        std::vector<float> buf(96000 * 2);
        for (int i = 0; i < 96000; ++i) {
            const float v = static_cast<float>(0.5 * std::sin(2 * M_PI * 220.0 * i / 96000.0));
            buf[static_cast<size_t>(i) * 2] = v;
            buf[static_cast<size_t>(i) * 2 + 1] = v;
        }
        fx.process(buf.data(), 96000, 2);
        bool finite = true;
        for (float s : buf) {
            if (!std::isfinite(s) || std::fabs(s) > 4.0f) { finite = false; break; }
        }
        printf("AutoWah @96kHz stereo stabil -> %s\n", finite ? "OK" : "GAGAL");
        pass += finite; total++;
    }

    // 4) Diam tetap diam (tidak menambah derau).
    {
        AutoWah fx;
        fx.prepare(sr, ch);
        fx.setEnabled(true);
        std::vector<float> silence(4096, 0.0f);
        fx.process(silence.data(), 4096, ch);
        float peak = 0.0f;
        for (float s : silence) peak = std::max(peak, std::fabs(s));
        const bool ok = peak < 1e-6f;
        printf("AutoWah diam: puncak=%.2e -> %s\n", peak, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    printf("\nAuto Wah: %d/%d lolos\n", pass, total);
    return pass == total ? 0 : 1;
}
