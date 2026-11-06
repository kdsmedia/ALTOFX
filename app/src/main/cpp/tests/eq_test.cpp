#include "Equalizer.h"
#include <cstdio>
#include <cmath>
#include <vector>
using namespace alto;

// Ukur amplitudo respons steady-state pada frekuensi tertentu.
static double response(Equalizer& eq, double sr, double f, int ch) {
    const int n = 16384;
    std::vector<float> buf(n);
    for (int i = 0; i < n; ++i) buf[i] = std::sin(2*M_PI*f*i/sr);
    eq.process(buf.data(), n, ch);
    // Ukur RMS pada paruh akhir (steady state).
    double sum = 0;
    for (int i = n/2; i < n; ++i) sum += buf[i]*buf[i];
    return std::sqrt(sum / (n/2)) * std::sqrt(2.0); // amplitudo puncak
}

int main() {
    const double sr = 48000.0;
    const int ch = 1;
    int pass = 0, total = 0;

    struct Band { const char* name; int index; double freq; float gainDb; };
    // Uji boost dan cut tiap band. Shelf diukur jauh di dalam pita shelf
    // (bass 30 Hz, treble 12 kHz) karena tepat di frekuensi sudut shelf hanya
    // memberi setengah gain dalam dB.
    Band tests[] = {
        {"Bass boost", 0, 30.0, +12.0f},
        {"Bass cut", 0, 30.0, -12.0f},
        {"Mid boost", 1, 800.0, +12.0f},
        {"Mid cut", 1, 800.0, -12.0f},
        {"Treble boost", 2, 12000.0, +12.0f},
        {"Treble cut", 2, 12000.0, -12.0f},
    };

    for (auto& t : tests) {
        Equalizer eq;
        eq.prepare(sr, ch);
        eq.setParam(0, 0); eq.setParam(1, 0); eq.setParam(2, 0);
        eq.setParam(t.index, t.gainDb);
        eq.setEnabled(true);
        double amp = response(eq, sr, t.freq, ch);
        double expected = std::pow(10.0, t.gainDb / 20.0);
        double error = std::fabs(amp - expected) / expected;
        bool ok = error < 0.15; // toleransi 15%
        printf("%-13s gain=%+.0f dB amp=%.3f harapan=%.3f err=%.1f%% -> %s\n",
               t.name, t.gainDb, amp, expected, error*100, ok ? "OK" : "GAGAL");
        if (ok) pass++;
        total++;
    }

    // Flat (semua 0 dB) harus melewatkan sinyal tanpa perubahan berarti.
    {
        Equalizer eq;
        eq.prepare(sr, ch);
        eq.setParam(0, 0); eq.setParam(1, 0); eq.setParam(2, 0);
        eq.setEnabled(true);
        double amp = response(eq, sr, 800.0, ch);
        bool ok = std::fabs(amp - 1.0) < 0.05;
        printf("%-13s amp=%.3f (harapan ~1.0) -> %s\n", "Flat", amp, ok ? "OK" : "GAGAL");
        if (ok) pass++;
        total++;
    }

    // Perubahan besar mendadak (seperti memuat preset) tidak boleh menimbulkan
    // lompatan sampel besar. Gain dihaluskan per sampel.
    {
        Equalizer eq;
        eq.prepare(sr, ch);
        eq.setParam(0, 0); eq.setParam(1, 0); eq.setParam(2, 0);
        eq.setEnabled(true);
        const int n = 48000;               // 1 detik sinyal kontinu 60 Hz
        std::vector<float> buf(n);
        for (int i = 0; i < n; ++i) buf[i] = std::sin(2*M_PI*60.0*i/sr);
        float maxStep = 0.0f;
        float prev = 0.0f;
        bool first = true;
        for (int off = 0; off < n; off += 128) {
            // Ubah bass ke +18 dB mendadak di tengah (seperti memuat preset).
            if (off == n / 4) eq.setParam(0, 18.0f);
            const int m = (n - off < 128) ? (n - off) : 128;
            eq.process(buf.data() + off, m, ch);
            for (int i = 0; i < m; ++i) {
                if (!first) maxStep = std::fmax(maxStep, std::fabs(buf[off+i] - prev));
                prev = buf[off+i];
                first = false;
            }
        }
        // Sinyal 60 Hz amplitudo 1: langkah antar-sampel wajar < 0.2.
        bool ok = std::isfinite(maxStep) && maxStep < 0.2f;
        printf("%-13s langkah maks=%.4f (tanpa klik) -> %s\n",
               "Preset jump", maxStep, ok ? "OK" : "GAGAL");
        if (ok) pass++;
        total++;
    }

    printf("\nEQ: %d/%d lolos\n", pass, total);
    return pass == total ? 0 : 1;
}
