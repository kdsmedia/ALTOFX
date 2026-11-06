#include "GainStage.h"
#include <cstdio>
#include <cmath>
#include <vector>
using namespace alto;

int main() {
    const double sr = 48000.0;
    int pass = 0, total = 0;

    // dB <-> linier.
    {
        bool ok = std::fabs(GainStage::dbToLinear(0.0f) - 1.0f) < 1e-4f
               && std::fabs(GainStage::dbToLinear(6.0f) - 1.9953f) < 1e-3f
               && std::fabs(GainStage::dbToLinear(-6.0f) - 0.5012f) < 1e-3f;
        printf("dB<->linier 0dB=%.3f 6dB=%.3f -6dB=%.3f -> %s\n",
               GainStage::dbToLinear(0.f), GainStage::dbToLinear(6.f),
               GainStage::dbToLinear(-6.f), ok?"OK":"GAGAL");
        pass += ok; total++;
    }

    // Gain diterapkan setelah perataan mapan.
    {
        GainStage g; g.prepare(sr); g.setTargetDb(-6.0f); g.snapToTarget();
        std::vector<float> buf(2048, 1.0f);
        g.process(buf.data(), 2048, 1);
        float expected = GainStage::dbToLinear(-6.0f);
        bool ok = std::fabs(buf[2047] - expected) < 1e-3f;
        printf("Gain -6dB diterapkan: %.4f (harapan %.4f) -> %s\n",
               buf[2047], expected, ok?"OK":"GAGAL");
        pass += ok; total++;
    }

    // Perubahan gain tidak langsung melonjak (smoothing mencegah klik).
    {
        GainStage g; g.prepare(sr, 20.0f); g.setTargetDb(0.0f); g.snapToTarget();
        g.setTargetDb(-24.0f); // target berubah tajam
        std::vector<float> buf(256, 1.0f);
        g.process(buf.data(), 256, 1);
        float first = buf[0];
        // Sampel pertama harus masih dekat 1.0 (belum langsung turun ke target).
        bool ok = first > 0.9f;
        printf("Smoothing: sampel pertama=%.4f (belum langsung turun) -> %s\n",
               first, ok?"OK":"GAGAL");
        pass += ok; total++;
    }

    // Perataan menuju target tanpa overshoot.
    {
        GainStage g; g.prepare(sr, 10.0f); g.setTargetLinear(1.0f); g.snapToTarget();
        g.setTargetLinear(0.25f);
        std::vector<float> buf(48000, 1.0f); // 1 detik
        g.process(buf.data(), 48000, 1);
        float target = 0.25f;
        bool ok = std::fabs(buf[47999] - target) < 0.01f;
        for (float v : buf) if (v < target - 1e-4f) { ok = false; break; } // tanpa overshoot
        printf("Konvergen ke target=%.3f akhir=%.4f tanpa overshoot -> %s\n",
               target, buf[47999], ok?"OK":"GAGAL");
        pass += ok; total++;
    }

    // Stereo: kedua kanal mendapat gain sama.
    {
        GainStage g; g.prepare(sr); g.setTargetLinear(0.5f); g.snapToTarget();
        std::vector<float> buf(100 * 2, 1.0f);
        g.process(buf.data(), 100, 2);
        bool ok = std::fabs(buf[0] - 0.5f) < 1e-4f && std::fabs(buf[1] - 0.5f) < 1e-4f;
        printf("Stereo gain: L=%.3f R=%.3f -> %s\n", buf[0], buf[1], ok?"OK":"GAGAL");
        pass += ok; total++;
    }

    printf("\nGain/limiter: %d/%d lolos\n", pass, total);
    return pass==total?0:1;
}
