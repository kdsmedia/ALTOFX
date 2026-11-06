#include "DriveEffect.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>
using namespace alto;

static double mag(const std::vector<float>& x, double sr, double f) {
    double re = 0, im = 0; int n = (int)x.size();
    for (int i = 0; i < n; ++i) { re += x[i]*std::cos(2*M_PI*f*i/sr); im += x[i]*std::sin(2*M_PI*f*i/sr); }
    return std::sqrt(re*re+im*im)/n;
}

int main() {
    const double sr = 48000.0;
    const int ch = 1, n = 8192;
    const double f0 = 220.0;

    struct Case { const char* name; DriveEffect::Shape shape; float scale; };
    Case cases[] = {
        {"Overdrive", DriveEffect::Shape::SoftTanh, 30.0f},
        {"Distortion", DriveEffect::Shape::HardClip, 120.0f},
        {"Fuzz", DriveEffect::Shape::Fuzz, 400.0f},
    };
    std::vector<std::vector<float>> outs;
    for (auto& c : cases) {
        DriveEffect fx(c.name, c.shape, c.scale);
        fx.prepare(sr, ch);
        fx.setParam(0, 0.8f); fx.setParam(1, 0.9f); fx.setParam(2, 0.5f); fx.setEnabled(true);
        std::vector<float> buf(n);
        for (int i = 0; i < n; ++i) buf[i] = 0.5f*std::sin(2*M_PI*f0*i/sr);
        for (int b = 0; b < 5; ++b) fx.process(buf.data(), n, ch);
        double odd = mag(buf,sr,f0*3)+mag(buf,sr,f0*5);
        double even = mag(buf,sr,f0*2)+mag(buf,sr,f0*4);
        printf("%-11s even/odd=%.3f (genap=%.4f ganjil=%.4f)\n", c.name, even/odd, even, odd);
        outs.push_back(buf);
    }
    // Perbedaan bentuk gelombang antar efek (RMS selisih).
    for (size_t i = 0; i < outs.size(); ++i)
        for (size_t j = i+1; j < outs.size(); ++j) {
            double d = 0;
            for (int k = 0; k < n; ++k) { double e = outs[i][k]-outs[j][k]; d += e*e; }
            printf("Beda %s vs %s: RMS=%.4f -> %s\n", cases[i].name, cases[j].name,
                   std::sqrt(d/n), std::sqrt(d/n) > 0.01 ? "BERBEDA OK" : "SAMA ⚠️");
        }
    return 0;
}
