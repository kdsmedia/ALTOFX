#include "Delay.h"
#include "Reverb.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>
using namespace alto;

int main() {
    const double sr = 48000.0;
    const int ch = 1;
    int pass = 0, total = 0;

    // Delay: impuls pada t=0 harus muncul kembali sekitar waktu tunda.
    {
        Delay fx; fx.prepare(sr, ch);
        fx.setParam(0, 200.0f); // 200 ms
        fx.setParam(1, 0.5f);
        fx.setParam(2, 0.8f);
        fx.setEnabled(true);
        const int n = 48000; // 1 detik
        std::vector<float> buf(n, 0.0f);
        buf[0] = 1.0f;
        fx.process(buf.data(), n, ch);

        int expected = (int)(0.200 * sr);
        // Cari puncak pada rentang setelah impuls awal.
        int peakIndex = 0; float peak = 0;
        for (int i = expected - 200; i < expected + 200 && i < n; ++i) {
            if (std::fabs(buf[i]) > peak) { peak = std::fabs(buf[i]); peakIndex = i; }
        }
        int errorMs = std::abs(peakIndex - expected) * 1000 / (int)sr;
        bool ok = peak > 0.1f && errorMs <= 3;
        printf("Delay   gema di %d ms (harapan %d ms, galat %d ms, amp %.2f) -> %s\n",
               peakIndex*1000/(int)sr, expected*1000/(int)sr, errorMs, peak, ok?"OK":"GAGAL");
        pass += ok; total++;
    }

    // Reverb: ekor harus bertahan setelah input berhenti, lalu meredup.
    {
        Reverb fx; fx.prepare(sr, ch);
        fx.setParam(0, 0.8f); fx.setParam(1, 0.8f); fx.setParam(2, 0.3f); fx.setParam(3, 0.8f);
        fx.setEnabled(true);
        const int n = 96000; // 2 detik
        std::vector<float> buf(n, 0.0f);
        // Burst singkat 50 ms.
        for (int i = 0; i < (int)(0.05*sr); ++i) buf[i] = 0.8f * std::sin(2*M_PI*440*i/sr);
        fx.process(buf.data(), n, ch);

        auto energy = [&](int from, int to) {
            double s = 0; for (int i = from; i < to; ++i) s += buf[i]*buf[i];
            return std::sqrt(s / (to - from));
        };
        double early = energy((int)(0.3*sr), (int)(0.4*sr));
        double late = energy((int)(1.2*sr), (int)(1.3*sr));
        bool ok = early > 1e-3 && late < early && late > 1e-5; // ada ekor & meredup
        printf("Reverb  ekor awal=%.5f akhir=%.5f (meredup) -> %s\n",
               early, late, ok?"OK":"GAGAL");
        pass += ok; total++;
    }

    // Reverb decay tinggi harus menghasilkan ekor lebih panjang.
    {
        auto tail = [&](float decay) {
            Reverb fx; fx.prepare(sr, ch);
            fx.setParam(0, 0.5f); fx.setParam(1, decay); fx.setParam(2, 0.2f); fx.setParam(3, 1.0f);
            fx.setEnabled(true);
            const int n = 96000;
            std::vector<float> buf(n, 0.0f);
            for (int i = 0; i < (int)(0.05*sr); ++i) buf[i] = 0.8f * std::sin(2*M_PI*440*i/sr);
            fx.process(buf.data(), n, ch);
            double s = 0; for (int i = (int)(0.9*sr); i < (int)(1.0*sr); ++i) s += buf[i]*buf[i];
            return std::sqrt(s / (0.1*sr));
        };
        double low = tail(0.1f), high = tail(0.9f);
        bool ok = high > low * 10.0; // ekor decay tinggi jauh lebih panjang
        printf("Reverb  decay 0.1=%.6f decay 0.9=%.6f (ekor lebih panjang) -> %s\n",
               low, high, ok?"OK":"GAGAL");
        pass += ok; total++;
    }

    // DelayLine: indeks tidak boleh keluar batas saat buffer dibaca lama
    // (regresi bug presisi float pada buffer besar, ~1,2 detik @48 kHz).
    {
        DelayLine line;
        line.prepare(sr, 1200.0f + 10.0f, 2);
        const int size = static_cast<int>(1210.0 * 0.001 * sr) + 4;
        bool ok = true;
        for (int i = 0; i < size * 3; ++i) {
            line.write(1.0f, 0);
            // Baca tepat pada panjang buffer penuh: dulu dapat menghasilkan
            // indeks == size (di luar batas).
            const float v = line.read(static_cast<float>(size - 2), 0);
            if (!std::isfinite(v)) { ok = false; break; }
            line.advance();
        }
        printf("DelayLine  3x panjang buffer tanpa OOB -> %s\n", ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    printf("\nRuang/waktu: %d/%d lolos\n", pass, total);
    return pass == total ? 0 : 1;
}
