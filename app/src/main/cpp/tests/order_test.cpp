#include "Effects.h"
#include <cstdio>
#include <cmath>
#include <vector>
using namespace alto;

static bool sameOrder(const std::array<EffectType, EffectChain::kEffectCount>& a,
                      const std::array<EffectType, EffectChain::kEffectCount>& b) {
    for (int i = 0; i < EffectChain::kEffectCount; ++i)
        if (a[i] != b[i]) return false;
    return true;
}

int main() {
    int pass = 0, total = 0;

    // Bawaan: urutan = indeks enum.
    {
        EffectChain chain;
        auto order = chain.order();
        bool ok = true;
        for (int i = 0; i < EffectChain::kEffectCount; ++i)
            ok = ok && (order[i] == static_cast<EffectType>(i));
        printf("Bawaan  urutan=indeks enum -> %s\n", ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // moveEffect: menukar dua slot bertetangga.
    {
        EffectChain chain;
        const auto before = chain.order();
        const int pos = 6;  // Equalizer (indeks enum) pada urutan bawaan.
        chain.moveEffect(EffectType::Equalizer, -1); // EQ naik satu langkah
        auto order = chain.order();
        bool ok = order[pos - 1] == EffectType::Equalizer &&
                  order[pos] == before[pos - 1];
        printf("move    EQ naik: slot%d=%d slot%d=%d -> %s\n",
               pos - 1, (int)order[pos - 1], pos, (int)order[pos],
               ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // Batas: efek pertama tidak bisa naik, efek terakhir tidak bisa turun.
    {
        EffectChain chain;
        auto before = chain.order();
        chain.moveEffect(EffectType::NoiseGate, -1);
        chain.moveEffect(EffectType::Reverb, +1);
        bool ok = sameOrder(before, chain.order());
        printf("batas   tidak keluar rentang -> %s\n", ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // resetOrder mengembalikan ke bawaan setelah diubah.
    {
        EffectChain chain;
        chain.moveEffect(EffectType::Reverb, -3);
        chain.resetOrder();
        auto order = chain.order();
        bool ok = true;
        for (int i = 0; i < EffectChain::kEffectCount; ++i)
            ok = ok && (order[i] == static_cast<EffectType>(i));
        printf("reset   kembali ke bawaan -> %s\n", ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // Urutan benar-benar mengubah hasil DSP: distorsi lalu EQ vs EQ lalu distorsi
    // memberi keluaran berbeda (operasi tidak komutatif).
    {
        const double sr = 48000.0;
        const int n = 4096;
        auto render = [&](bool eqFirst) {
            EffectChain chain;
            chain.prepare(sr, 1);
            // Aktifkan Distortion dan Equalizer saja.
            chain.setEnabled(EffectType::Distortion, true);
            chain.setEnabled(EffectType::Equalizer, true);
            chain.setParam(EffectType::Distortion, 0, 0.9f); // gain
            chain.setParam(EffectType::Distortion, 2, 0.9f); // level
            chain.setParam(EffectType::Equalizer, 2, 10.0f); // treble boost besar
            if (eqFirst) {
                chain.moveEffect(EffectType::Equalizer, -1); // EQ sebelum distorsi
                chain.moveEffect(EffectType::Equalizer, -1);
                chain.moveEffect(EffectType::Equalizer, -1);
                chain.moveEffect(EffectType::Equalizer, -1);
            }
            std::vector<float> buf(n);
            for (int i = 0; i < n; ++i) buf[i] = 0.6f * std::sin(2*M_PI*440*i/sr);
            // Beberapa blok agar smoothing mapan.
            for (int b = 0; b < 8; ++b) chain.process(buf.data(), n, 1);
            double s = 0; for (float v : buf) s += v*v;
            return std::sqrt(s / n);
        };
        double distFirst = render(false);
        double eqFirst = render(true);
        bool ok = std::fabs(distFirst - eqFirst) > 1e-3;
        printf("urutan  dist->EQ=%.5f EQ->dist=%.5f (berbeda) -> %s\n",
               distFirst, eqFirst, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }


    // setOrder: menerapkan permutasi lengkap sekaligus.
    {
        EffectChain chain;
        std::array<EffectType, EffectChain::kEffectCount> target{};
        for (int i = 0; i < EffectChain::kEffectCount; ++i)
            target[i] = static_cast<EffectType>(EffectChain::kEffectCount - 1 - i);
        chain.setOrder(target);
        auto order = chain.order();
        bool ok = true;
        for (int i = 0; i < EffectChain::kEffectCount; ++i)
            ok = ok && (order[i] == target[i]);
        printf("setOrder permutasi terbalik diterapkan -> %s\n", ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    printf("\nUrutan efek: %d/%d lolos\n", pass, total);
    return pass == total ? 0 : 1;
}
