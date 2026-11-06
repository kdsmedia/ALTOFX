#include "Effects.h"
#include "GainStage.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

using namespace alto;

// Mengaktifkan seluruh 12 efek seperti pedalboard penuh.
static void enableAll(EffectChain& c) {
    for (int i = 0; i < EffectChain::kEffectCount; ++i) {
        c.setEnabled(static_cast<EffectType>(i), true);
    }
}

static void setDefaults(EffectChain& c) {
    c.setParam(EffectType::NoiseGate, 0, -50.0f);
    c.setParam(EffectType::Compressor, 0, -18.0f);
    c.setParam(EffectType::Overdrive, 0, 0.5f);
    c.setParam(EffectType::Distortion, 0, 0.5f);
    c.setParam(EffectType::Fuzz, 0, 0.5f);
    c.setParam(EffectType::Equalizer, 0, 0.0f);
    c.setParam(EffectType::Equalizer, 1, 0.0f);
    c.setParam(EffectType::Equalizer, 2, 0.0f);
    c.setParam(EffectType::Chorus, 0, 0.5f);
    c.setParam(EffectType::Flanger, 0, 0.5f);
    c.setParam(EffectType::Phaser, 0, 0.5f);
    c.setParam(EffectType::Tremolo, 0, 0.5f);
    c.setParam(EffectType::Delay, 0, 300.0f);
    c.setParam(EffectType::Delay, 1, 0.3f);
    c.setParam(EffectType::Reverb, 0, 0.3f);
}

// Sinyal gitar sintetis: pick + harmonik dengan peluruhan eksponensial.
static std::vector<float> guitar(double sr, int n, double f0, unsigned seed) {
    std::vector<float> out(n);
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> noise(-0.02f, 0.02f);
    for (int i = 0; i < n; ++i) {
        const double t = i / sr;
        const double env = std::exp(-2.5 * t);
        const double v = std::sin(2 * M_PI * f0 * t) + 0.5 * std::sin(4 * M_PI * f0 * t)
                       + 0.25 * std::sin(6 * M_PI * f0 * t);
        out[i] = static_cast<float>(v * env * 0.4) + noise(rng);
    }
    return out;
}

static std::vector<float> toStereo(const std::vector<float>& mono) {
    std::vector<float> stereo(mono.size() * 2);
    for (size_t i = 0; i < mono.size(); ++i) {
        stereo[i * 2] = mono[i];
        stereo[i * 2 + 1] = mono[i];
    }
    return stereo;
}

static bool finiteAndBounded(const std::vector<float>& v, float limit) {
    for (float s : v) {
        if (!std::isfinite(s) || std::fabs(s) >= limit) {
            return false;
        }
    }
    return true;
}

int main() {
    const double sr = 48000.0;
    const int ch = 2;
    int pass = 0, total = 0;

    // 1) Seluruh 12 efek aktif: output berhingga dan terbatas.
    {
        EffectChain c; c.prepare(sr, ch); enableAll(c); setDefaults(c);
        auto stereo = toStereo(guitar(sr, 48000, 82.41, 1));
        c.process(stereo.data(), 48000, ch);
        bool ok = finiteAndBounded(stereo, 10.0f);
        printf("12 efek aktif (satu blok besar) -> %s\n", ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 2) Blok 128 frame (seperti callback nyata): tetap stabil.
    {
        EffectChain c; c.prepare(sr, ch); enableAll(c); setDefaults(c);
        auto stereo = toStereo(guitar(sr, 96000, 110.0, 2));
        for (int off = 0; off < 96000; off += 128) {
            const int n = std::min(128, 96000 - off);
            c.process(stereo.data() + off * ch, n, ch);
        }
        bool ok = finiteAndBounded(stereo, 10.0f);
        printf("Blok 128 (750 callback) -> %s\n", ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 3) Bypass global: output identik dengan input (dry).
    {
        EffectChain c; c.prepare(sr, ch); enableAll(c); setDefaults(c);
        c.setGlobalBypass(true);
        auto stereo = toStereo(guitar(sr, 4096, 146.83, 3));
        const std::vector<float> expect = stereo;
        c.process(stereo.data(), 4096, ch);
        float maxDiff = 0.0f;
        for (size_t i = 0; i < stereo.size(); ++i) {
            maxDiff = std::max(maxDiff, std::fabs(stereo[i] - expect[i]));
        }
        bool ok = maxDiff < 1e-6f;
        printf("Bypass global dry: selisih maks=%.2e -> %s\n", maxDiff, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 4) Mute master: meredup ke nol dengan perataan, tanpa potong mendadak.
    {
        GainStage g; g.prepare(sr, 20.0f); g.setTargetLinear(1.0f); g.snapToTarget();
        g.setMuted(true);
        std::vector<float> buf(48000, 1.0f);
        g.process(buf.data(), 48000, 1);
        bool noJump = buf[0] > 0.9f;
        bool reached = buf[47999] < 1e-3f;
        bool ok = noJump && reached;
        printf("Mute halus: sampel1=%.4f akhir=%.6f -> %s\n",
               buf[0], buf[47999], ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 5) Perubahan parameter saat berjalan tetap wajar (tidak meledak).
    {
        EffectChain c; c.prepare(sr, ch); enableAll(c); setDefaults(c);
        auto stereo = toStereo(guitar(sr, 24000, 98.0, 4));
        float maxStep = 0.0f;
        for (int off = 0; off < 24000; off += 128) {
            const int n = std::min(128, 24000 - off);
            if ((off / 128) % 10 == 0) {
                const float v = 0.3f + 0.4f * std::sin(off * 0.001f);
                c.setParam(EffectType::Delay, 1, v);
                c.setParam(EffectType::Tremolo, 0, v);
                c.setParam(EffectType::Overdrive, 0, v);
            }
            float prev = (off == 0) ? 0.0f : stereo[(off - 1) * ch];
            c.process(stereo.data() + off * ch, n, ch);
            for (int i = 0; i < n; ++i) {
                const float v = stereo[(off + i) * ch];
                maxStep = std::max(maxStep, std::fabs(v - prev));
                prev = v;
            }
        }
        bool ok = std::isfinite(maxStep) && maxStep < 2.0f;
        printf("Ubah param saat berjalan: langkah maks=%.4f -> %s\n",
               maxStep, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 6) Mengubah urutan efek saat berjalan: tetap stabil.
    {
        EffectChain c; c.prepare(sr, ch); enableAll(c); setDefaults(c);
        auto stereo = toStereo(guitar(sr, 12000, 82.41, 5));
        for (int off = 0; off < 12000; off += 128) {
            const int n = std::min(128, 12000 - off);
            if ((off / 128) % 20 == 0) {
                c.moveEffect(EffectType::Reverb, -1);
            }
            c.process(stereo.data() + off * ch, n, ch);
        }
        bool ok = finiteAndBounded(stereo, 10.0f);
        printf("Ubah urutan saat berjalan -> %s\n", ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 7) Stabilitas jangka panjang: 60 detik audio pada blok 128 dengan 12 efek.
    //    Mendeteksi NaN/Inf, ledakan level, atau penyimpangan DC yang menumpuk.
    {
        EffectChain c; c.prepare(sr, ch); enableAll(c); setDefaults(c);
        const int totalFrames = static_cast<int>(sr) * 60;  // 60 detik
        const int block = 128;
        std::vector<float> buf(block * ch);
        std::vector<float> src(block * ch);
        std::mt19937 rng(7);
        std::uniform_real_distribution<float> noise(-0.02f, 0.02f);
        bool finite = true;
        float peak = 0.0f;
        double dcSum = 0.0;
        long long dcCount = 0;
        int callbacks = 0;
        for (int off = 0; off < totalFrames; off += block) {
            const int n = std::min(block, totalFrames - off);
            // Sinyal petikan berganti-ganti, seperti gitar dimainkan.
            const double t0 = off / sr;
            const double f0 = (callbacks / 200) % 2 == 0 ? 82.41 : 146.83;
            for (int i = 0; i < n; ++i) {
                const double t = t0 + i / sr;
                const double env = std::exp(-2.0 * std::fmod(t, 2.0));
                const double v = std::sin(2 * M_PI * f0 * t) + 0.4 * std::sin(4 * M_PI * f0 * t);
                const float sample = static_cast<float>(v * env * 0.35) + noise(rng);
                src[i * ch] = sample;
                src[i * ch + 1] = sample;
            }
            std::copy(src.begin(), src.begin() + n * ch, buf.begin());
            // Sesekali ubah parameter seperti pemain memutar knob.
            if (callbacks % 500 == 0) {
                c.setParam(EffectType::Delay, 1, 0.2f + 0.3f * std::sin(callbacks * 0.01f));
                c.setParam(EffectType::Reverb, 0, 0.2f + 0.2f * std::cos(callbacks * 0.01f));
            }
            c.process(buf.data(), n, ch);
            for (int i = 0; i < n * ch; ++i) {
                const float v = buf[i];
                if (!std::isfinite(v)) { finite = false; break; }
                peak = std::max(peak, std::fabs(v));
                dcSum += v; dcCount++;
            }
            if (!finite) break;
            callbacks++;
        }
        const double dcMean = dcCount > 0 ? dcSum / dcCount : 0.0;
        bool ok = finite && peak < 10.0f && std::fabs(dcMean) < 0.05;
        printf("Stabilitas 60s (%d callback): berhingga=%d puncak=%.4f dc=%.5f -> %s\n",
               callbacks, finite, peak, dcMean, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    printf("\nIntegrasi: %d/%d lolos\n", pass, total);
    return pass == total ? 0 : 1;
}
