#include "Effects.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace alto;

// Nama efek untuk keluaran uji (EffectChain::effect bersifat privat).
static const char* effectName(EffectType type) {
    switch (type) {
        case EffectType::NoiseGate: return "NoiseGate";
        case EffectType::Compressor: return "Compressor";
        case EffectType::Overdrive: return "Overdrive";
        case EffectType::Distortion: return "Distortion";
        case EffectType::Fuzz: return "Fuzz";
        case EffectType::Amp: return "Amp";
        case EffectType::Equalizer: return "Equalizer";
        case EffectType::Chorus: return "Chorus";
        case EffectType::Flanger: return "Flanger";
        case EffectType::Phaser: return "Phaser";
        case EffectType::Tremolo: return "Tremolo";
        case EffectType::Delay: return "Delay";
        case EffectType::Reverb: return "Reverb";
        case EffectType::AutoWah: return "AutoWah";
        default: return "?";
    }
}

// Sinyal petikan: kuat lalu meluruh, sehingga efek bergantung level (gate,
// compressor) juga terpicu.
static std::vector<float> pluck(int frames, int channels) {
    std::vector<float> buffer(static_cast<size_t>(frames) * channels);
    for (int i = 0; i < frames; ++i) {
        const double t = i / 48000.0;
        const double env = std::exp(-6.0 * t);
        const double v = std::sin(2 * M_PI * 220.0 * t) + 0.3 * std::sin(2 * M_PI * 440.0 * t);
        const float s = static_cast<float>(v * env * 0.5);
        for (int c = 0; c < channels; ++c) buffer[i * channels + c] = s;
    }
    return buffer;
}

static double rmsDiff(const std::vector<float>& a, const std::vector<float>& b) {
    double sum = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        const double d = a[i] - b[i];
        sum += d * d;
    }
    return std::sqrt(sum / static_cast<double>(a.size()));
}

static bool bitEqual(const std::vector<float>& a, const std::vector<float>& b) {
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// Parameter kuat per efek agar pengaruhnya jelas terukur.
static void configureStrong(EffectChain& chain, EffectType type) {
    switch (type) {
        case EffectType::NoiseGate:
            chain.setParam(type, 0, -25.0f);  // threshold
            chain.setParam(type, 1, 50.0f);   // release
            chain.setParam(type, 2, 1.0f);    // attack
            break;
        case EffectType::Compressor:
            chain.setParam(type, 0, -30.0f);
            chain.setParam(type, 1, 10.0f);
            chain.setParam(type, 2, 5.0f);
            chain.setParam(type, 3, 100.0f);
            chain.setParam(type, 4, 12.0f);   // makeup
            break;
        case EffectType::Overdrive:
        case EffectType::Distortion:
        case EffectType::Fuzz:
            chain.setParam(type, 0, 0.9f);
            chain.setParam(type, 1, 0.5f);
            chain.setParam(type, 2, 0.8f);
            break;
        case EffectType::Amp:
            chain.setParam(type, 0, 2.0f);   // Lead
            chain.setParam(type, 1, 18.0f);
            chain.setParam(type, 2, 8.0f);
            chain.setParam(type, 3, 6.0f);
            chain.setParam(type, 4, 8.0f);
            chain.setParam(type, 5, 6.0f);
            chain.setParam(type, 6, 6.0f);
            break;
        case EffectType::Equalizer:
            chain.setParam(type, 0, 12.0f);
            chain.setParam(type, 1, -6.0f);
            chain.setParam(type, 2, 9.0f);
            break;
        case EffectType::Chorus:
        case EffectType::Flanger:
        case EffectType::Phaser:
            chain.setParam(type, 0, 2.0f);    // rate
            chain.setParam(type, 1, 0.9f);    // depth
            chain.setParam(type, 2, 0.6f);    // feedback (flanger/phaser)
            chain.setParam(type, 3, 0.5f);    // mix
            break;
        case EffectType::Tremolo:
            chain.setParam(type, 0, 5.0f);    // rate
            chain.setParam(type, 1, 1.0f);    // depth
            break;
        case EffectType::Delay:
            chain.setParam(type, 0, 200.0f);
            chain.setParam(type, 1, 0.5f);
            chain.setParam(type, 2, 0.5f);
            break;
        case EffectType::Reverb:
            chain.setParam(type, 0, 0.7f);
            chain.setParam(type, 1, 0.7f);
            chain.setParam(type, 2, 0.5f);
            chain.setParam(type, 3, 0.5f);
            break;
        case EffectType::AutoWah:
            chain.setParam(type, 0, 0.8f);
            chain.setParam(type, 1, 0.9f);
            chain.setParam(type, 2, 8.0f);
            chain.setParam(type, 3, 1.0f);
            break;
        default:
            break;
    }
}

int main() {
    const int ch = 2;
    const int frames = 24000;  // 0.5 detik
    int pass = 0, total = 0;

    const auto input = pluck(frames, ch);

    // 1) Setiap efek aktif mengubah sinyal (efeknya nyata, bukan passthrough).
    for (int i = 0; i < EffectChain::kEffectCount; ++i) {
        const auto type = static_cast<EffectType>(i);
        EffectChain chain; chain.prepare(48000.0, ch);
        configureStrong(chain, type);
        chain.setEnabled(type, true);
        auto out = input;
        chain.process(out.data(), frames, ch);
        const double diff = rmsDiff(input, out);
        const bool ok = std::isfinite(diff) && diff > 0.002;
        printf("%-12s aktif: RMS selisih=%.4f -> %s\n",
               effectName(type), diff, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 2) Bypass tiap efek: output identik bit-per-bit dengan input.
    for (int i = 0; i < EffectChain::kEffectCount; ++i) {
        const auto type = static_cast<EffectType>(i);
        EffectChain chain; chain.prepare(48000.0, ch);
        configureStrong(chain, type);
        chain.setEnabled(type, false);
        auto out = input;
        chain.process(out.data(), frames, ch);
        const bool ok = bitEqual(input, out);
        printf("%-12s bypass: identik=%d -> %s\n",
               effectName(type), bitEqual(input, out), ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 3) Kombinasi: dua efek aktif berbeda dari masing-masing sendiri.
    {
        EffectChain chain; chain.prepare(48000.0, ch);
        configureStrong(chain, EffectType::Distortion);
        configureStrong(chain, EffectType::Delay);
        chain.setEnabled(EffectType::Distortion, true);
        chain.setEnabled(EffectType::Delay, true);
        auto out = input;
        chain.process(out.data(), frames, ch);
        const double diff = rmsDiff(input, out);
        bool ok = std::isfinite(diff) && diff > 0.002;
        printf("Kombinasi Distortion+Delay: RMS selisih=%.4f -> %s\n",
               diff, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    printf("\nEfek satu per satu & bypass: %d/%d lolos\n", pass, total);
    return pass == total ? 0 : 1;
}
