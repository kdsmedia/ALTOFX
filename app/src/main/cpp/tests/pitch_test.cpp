#include "PitchDetector.h"
#include <cstdio>
#include <cmath>
#include <vector>
using namespace alto;

int main() {
    const double sr = 48000.0;
    int pass = 0, total = 0;
    struct Case { const char* name; float freq; };
    // Nada standar gitar (EADGBE).
    Case cases[] = {
        {"E2", 82.41f}, {"A2", 110.00f}, {"D3", 146.83f},
        {"G3", 196.00f}, {"B3", 246.94f}, {"E4", 329.63f},
    };
    for (auto& c : cases) {
        PitchDetector d; d.prepare(sr); d.setActive(true);
        // 4096 sampel sinyal dengan harmonik agar mirip gitar.
        std::vector<float> buf(4096);
        for (int i = 0; i < 4096; ++i) {
            buf[i] = 0.6f*std::sin(2*M_PI*c.freq*i/sr)
                   + 0.25f*std::sin(4*M_PI*c.freq*i/sr)
                   + 0.12f*std::sin(6*M_PI*c.freq*i/sr);
        }
        d.process(buf.data(), 4096, 1);
        float f = d.frequency();
        float cents = d.cents();
        float errCents = std::fabs(1200.0f*std::log2(f/c.freq));
        bool ok = f > 0 && errCents < 10.0f;
        printf("%-3s %.2f Hz -> %.2f Hz (galat %.1f cents, conf %.2f) %s\n",
               c.name, c.freq, f, errCents, d.confidence(), ok?"OK":"GAGAL");
        pass += ok; total++;
        (void)cents;
    }
    // Senyap harus menghasilkan tidak ada nada.
    {
        PitchDetector d; d.prepare(sr); d.setActive(true);
        std::vector<float> buf(4096, 0.0f);
        d.process(buf.data(), 4096, 1);
        bool ok = d.frequency() == 0.0f;
        printf("Senyap     freq=%.2f (tidak ada nada) %s\n", d.frequency(), ok?"OK":"GAGAL");
        pass += ok; total++;
    }
    // Nonaktif: tidak menganalisis.
    {
        PitchDetector d; d.prepare(sr); d.setActive(false);
        std::vector<float> buf(4096);
        for (int i = 0; i < 4096; ++i) buf[i] = 0.6f*std::sin(2*M_PI*110.0*i/sr);
        d.process(buf.data(), 4096, 1);
        bool ok = d.frequency() == 0.0f;
        printf("Nonaktif   freq=%.2f (dilewati) %s\n", d.frequency(), ok?"OK":"GAGAL");
        pass += ok; total++;
    }
    printf("\nTuner: %d/%d lolos\n", pass, total);
    return pass==total?0:1;
}
