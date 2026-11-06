#include "Chorus.h"
#include "Flanger.h"
#include "Phaser.h"
#include "Tremolo.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>
using namespace alto;

// Ukur kedalaman modulasi amplitudo (maks-min RMS dalam jendela kecil).
static float amplitudeRipple(const std::vector<float>& x, int window) {
    float lo = 1e9f, hi = 0.0f;
    for (size_t i = 0; i + window <= x.size(); i += window) {
        float sum = 0;
        for (int k = 0; k < window; ++k) sum += x[i + k] * x[i + k];
        float rms = std::sqrt(sum / window);
        lo = std::min(lo, rms);
        hi = std::max(hi, rms);
    }
    return hi > 1e-6f ? (hi - lo) / hi : 0.0f;
}

static float rms(const std::vector<float>& x) {
    double s = 0; for (float v : x) s += v * v;
    return (float)std::sqrt(s / x.size());
}

int main() {
    const double sr = 48000.0;
    const int ch = 1;
    const int n = 48000; // 1 detik
    int pass = 0, total = 0;

    // Tremolo: depth tinggi -> riak amplitudo besar dan stabil.
    {
        Tremolo fx; fx.prepare(sr, ch);
        fx.setParam(0, 5.0f); fx.setParam(1, 0.9f); fx.setEnabled(true);
        std::vector<float> buf(n);
        for (int i = 0; i < n; ++i) buf[i] = 0.5f * std::sin(2*M_PI*440*i/sr);
        fx.process(buf.data(), n, ch);
        float ripple = amplitudeRipple(buf, 480);
        bool ok = ripple > 0.5f;
        printf("Tremolo    riak amplitudo=%.2f -> %s\n", ripple, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // Chorus: mengubah sinyal (wet != dry) tapi tetap sebanding.
    {
        Chorus fx; fx.prepare(sr, ch);
        fx.setParam(0, 1.0f); fx.setParam(1, 0.8f); fx.setParam(2, 0.7f); fx.setEnabled(true);
        std::vector<float> buf(n);
        for (int i = 0; i < n; ++i) buf[i] = 0.5f * std::sin(2*M_PI*440*i/sr);
        fx.process(buf.data(), n, ch);
        float outRms = rms(buf);
        bool ok = outRms > 0.1f && outRms < 1.0f;
        printf("Chorus     RMS=%.3f -> %s\n", outRms, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // Flanger: dengan feedback tinggi harus tetap stabil (tidak meledak).
    {
        Flanger fx; fx.prepare(sr, ch);
        fx.setParam(0, 0.3f); fx.setParam(1, 0.9f); fx.setParam(2, 0.9f); fx.setParam(3, 0.5f);
        fx.setEnabled(true);
        std::vector<float> buf(n);
        for (int i = 0; i < n; ++i) buf[i] = 0.5f * std::sin(2*M_PI*440*i/sr);
        for (int b = 0; b < 5; ++b) fx.process(buf.data(), n, ch);
        float peak = 0; for (float v : buf) peak = std::max(peak, std::fabs(v));
        bool ok = peak < 3.0f;
        printf("Flanger    peak=%.3f (stabil) -> %s\n", peak, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // Phaser: menghasilkan takik yang bergerak -> amplitudo pada 1 kHz berubah.
    {
        Phaser fx; fx.prepare(sr, ch);
        fx.setParam(0, 0.5f); fx.setParam(1, 1.0f); fx.setParam(2, 0.7f); fx.setParam(3, 1.0f);
        fx.setEnabled(true);
        std::vector<float> buf(n);
        for (int i = 0; i < n; ++i) buf[i] = 0.5f * std::sin(2*M_PI*1000*i/sr);
        fx.process(buf.data(), n, ch);
        float ripple = amplitudeRipple(buf, 2400);
        bool ok = ripple > 0.05f;
        printf("Phaser     riak=%.3f (takik bergerak) -> %s\n", ripple, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    printf("\nModulasi: %d/%d lolos\n", pass, total);
    return pass == total ? 0 : 1;
}
