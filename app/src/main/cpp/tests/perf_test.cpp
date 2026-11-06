#include "PerformanceMonitor.h"
#include <cstdio>
#include <cmath>
using namespace alto;

int main() {
    int pass = 0, total = 0;

    // Beban dihitung terhadap anggaran waktu blok.
    {
        PerformanceMonitor m; m.prepare(48000.0);
        // Blok 128 frame = 2.6667 ms = 2666.7 us anggaran. Pakai 1333 us -> 0.5.
        for (int i = 0; i < 10; ++i) m.record(1333.0, 128);
        float load = m.averageLoad();
        bool ok = std::fabs(load - 0.5f) < 0.02f;
        printf("Beban 50%%: %.4f (harapan ~0.5) -> %s\n", load, ok?"OK":"GAGAL");
        pass += ok; total++;
    }

    // Puncak dicatat terpisah dari rata-rata.
    {
        PerformanceMonitor m; m.prepare(48000.0);
        m.record(100.0, 128);
        m.record(100.0, 128);
        m.record(2000.0, 128); // lonjakan
        bool ok = std::fabs(m.averageMicros() - (2200.0/3.0)) < 1.0
               && std::fabs(m.peakMicros() - 2000.0) < 1e-6;
        printf("Rata-rata=%.1f us puncak=%.0f us -> %s\n",
               m.averageMicros(), m.peakMicros(), ok?"OK":"GAGAL");
        pass += ok; total++;
    }

    // Beban puncak terdeteksi melebihi 1.0 saat callback melebihi anggaran.
    {
        PerformanceMonitor m; m.prepare(48000.0);
        m.record(5334.0, 128); // 2x anggaran
        bool ok = m.peakLoad() > 1.9f;
        printf("Overload puncak=%.2f (>1.9) -> %s\n", m.peakLoad(), ok?"OK":"GAGAL");
        pass += ok; total++;
    }

    // Reset mengosongkan statistik.
    {
        PerformanceMonitor m; m.prepare(48000.0);
        m.record(500.0, 128);
        m.reset();
        bool ok = m.callbacks() == 0 && m.averageMicros() == 0.0 && m.peakLoad() == 0.0f;
        printf("Reset: callbacks=%lld avg=%.1f -> %s\n",
               (long long)m.callbacks(), m.averageMicros(), ok?"OK":"GAGAL");
        pass += ok; total++;
    }

    // Sample rate berbeda -> anggaran berbeda (beban menyesuaikan).
    {
        PerformanceMonitor a; a.prepare(48000.0);
        PerformanceMonitor b; b.prepare(96000.0);
        a.record(2666.0, 128); // ~50% @48k
        b.record(2666.0, 128); // ~100% @96k (anggaran separuh)
        bool ok = b.averageLoad() > a.averageLoad() * 1.8f;
        printf("Sample rate: load48=%.2f load96=%.2f -> %s\n",
               a.averageLoad(), b.averageLoad(), ok?"OK":"GAGAL");
        pass += ok; total++;
    }

    printf("\nPerforma: %d/%d lolos\n", pass, total);
    return pass==total?0:1;
}
