#include "AntiFeedback.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace alto;

static double rms(const std::vector<float>& v, int from, int to) {
    double sum = 0.0;
    int count = 0;
    for (int i = from; i < to && i < static_cast<int>(v.size()); ++i) {
        sum += static_cast<double>(v[static_cast<size_t>(i)]) * v[static_cast<size_t>(i)];
        ++count;
    }
    return count > 0 ? std::sqrt(sum / count) : 0.0;
}

int main() {
    const double sr = 48000.0;
    const int ch = 2;
    const int block = 128;
    int pass = 0, total = 0;

    // 1) Nada feedback (sinus kuat & tetap) diredam setelah beberapa saat.
    {
        const int frames = static_cast<int>(sr) * 3;
        std::vector<float> buf(static_cast<size_t>(frames) * ch);
        for (int i = 0; i < frames; ++i) {
            const float v = static_cast<float>(0.5 * std::sin(2 * M_PI * 1000.0 * i / sr));
            buf[static_cast<size_t>(i) * ch] = v;
            buf[static_cast<size_t>(i) * ch + 1] = v;
        }
        const double inRms = rms(buf, 0, frames);

        AntiFeedback af;
        af.prepare(sr, ch);
        for (int off = 0; off < frames; off += block) {
            const int n = std::min(block, frames - off);
            af.process(buf.data() + static_cast<size_t>(off) * ch, n, ch);
        }
        // Ukur RMS di detik terakhir (setelah notch terbentuk).
        const double outRms = rms(buf, static_cast<int>(sr) * 2, frames);
        const bool ok = std::isfinite(outRms) && outRms < inRms * 0.5;
        printf("AntiFeedback redam nada tetap: in=%.4f out=%.4f -> %s\n",
               inRms, outRms, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 2) Petikan gitar yang meluruh TIDAK diredam (bukan feedback).
    {
        const int frames = static_cast<int>(sr) * 2;
        std::vector<float> in(static_cast<size_t>(frames) * ch);
        for (int i = 0; i < frames; ++i) {
            const double t = i / sr;
            const double env = std::exp(-3.0 * t);
            const float v = static_cast<float>(0.6 * env *
                (std::sin(2 * M_PI * 220.0 * t) + 0.4 * std::sin(4 * M_PI * 220.0 * t)));
            in[static_cast<size_t>(i) * ch] = v;
            in[static_cast<size_t>(i) * ch + 1] = v;
        }
        auto buf = in;
        AntiFeedback af;
        af.prepare(sr, ch);
        for (int off = 0; off < frames; off += block) {
            const int n = std::min(block, frames - off);
            af.process(buf.data() + static_cast<size_t>(off) * ch, n, ch);
        }
        float maxDiff = 0.0f;
        for (size_t i = 0; i < buf.size(); ++i) {
            maxDiff = std::max(maxDiff, std::fabs(buf[i] - in[i]));
        }
        // Perbedaan harus kecil: nada gitar yang meluruh dibiarkan utuh.
        const bool ok = std::isfinite(maxDiff) && maxDiff < 0.05f;
        printf("AntiFeedback biarkan petikan meluruh: selisih maks=%.4f -> %s\n",
               maxDiff, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 3) Diam tetap diam dan stabil pada 96 kHz.
    {
        AntiFeedback af;
        af.prepare(96000.0, 2);
        std::vector<float> silence(4096 * 2, 0.0f);
        af.process(silence.data(), 4096, 2);
        float peak = 0.0f;
        for (float s : silence) peak = std::max(peak, std::fabs(s));
        const bool ok = peak < 1e-6f;
        printf("AntiFeedback diam & @96kHz: puncak=%.2e -> %s\n", peak, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    printf("\nAnti-feedback: %d/%d lolos\n", pass, total);
    return pass == total ? 0 : 1;
}
