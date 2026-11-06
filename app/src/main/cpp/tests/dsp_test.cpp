#include "NoiseGate.h"
#include "Compressor.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>
using namespace alto;

int main() {
    const double sr = 48000.0;
    const int ch = 1;
    const int n = 4800;

    {
        NoiseGate gate;
        gate.prepare(sr, ch);
        gate.setParam(0, -30.0f);
        gate.setParam(1, 50.0f);
        gate.setParam(2, 2.0f);
        gate.setEnabled(true);
        std::vector<float> buf(n);
        for (int i = 0; i < n; ++i) buf[i] = 0.001f * std::sin(2*M_PI*220*i/sr);
        for (int b = 0; b < 20; ++b) gate.process(buf.data(), n, ch);
        float peak = 0; for (float s : buf) peak = std::max(peak, std::fabs(s));
        printf("NoiseGate (kecil): peak=%.6f -> %s\n", peak,
               peak < 0.0005f ? "TEREDAM OK" : "GAGAL");
    }
    {
        NoiseGate gate;
        gate.prepare(sr, ch);
        gate.setParam(0, -30.0f);
        gate.setEnabled(true);
        std::vector<float> buf(n);
        for (int i = 0; i < n; ++i) buf[i] = 0.5f * std::sin(2*M_PI*220*i/sr);
        for (int b = 0; b < 20; ++b) gate.process(buf.data(), n, ch);
        float peak = 0; for (float s : buf) peak = std::max(peak, std::fabs(s));
        printf("NoiseGate (besar): peak=%.4f -> %s\n", peak,
               peak > 0.4f ? "LOLOS OK" : "GAGAL");
    }
    {
        Compressor comp;
        comp.prepare(sr, ch);
        comp.setParam(0, -20.0f);
        comp.setParam(1, 4.0f);
        comp.setParam(2, 5.0f);
        comp.setParam(3, 100.0f);
        comp.setParam(4, 0.0f);
        comp.setEnabled(true);
        std::vector<float> buf(n);
        for (int i = 0; i < n; ++i) buf[i] = 0.9f * std::sin(2*M_PI*220*i/sr);
        for (int b = 0; b < 50; ++b) comp.process(buf.data(), n, ch);
        float peak = 0; for (float s : buf) peak = std::max(peak, std::fabs(s));
        printf("Compressor: in=0.900 out=%.4f -> %s\n", peak,
               (peak < 0.9f * 0.95f) ? "TERKOMPRESI OK" : "GAGAL");
    }
    return 0;
}
