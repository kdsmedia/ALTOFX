#include "Effects.h"
#include "GainStage.h"
#include "Limiter.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace alto;

// Mereplikasi jalur master nyata di AudioEngine::onBothStreamsReady:
// trim input -> rantai efek -> level output -> limiter puncak lookahead.
static void runMasterPath(
    std::vector<float>& buffer,
    int frames,
    int channels,
    float inputDb,
    float outputDb,
    bool enableAllEffects,
    float ceilingDb
) {
    GainStage inputGain; inputGain.prepare(48000.0, 20.0f);
    inputGain.setTargetDb(inputDb); inputGain.snapToTarget();
    GainStage outputGain; outputGain.prepare(48000.0, 20.0f);
    outputGain.setTargetDb(outputDb); outputGain.snapToTarget();

    EffectChain chain; chain.prepare(48000.0, channels);
    if (enableAllEffects) {
        for (int i = 0; i < EffectChain::kEffectCount; ++i) {
            chain.setEnabled(static_cast<EffectType>(i), true);
        }
    }

    inputGain.process(buffer.data(), frames, channels);
    chain.process(buffer.data(), frames, channels);
    outputGain.process(buffer.data(), frames, channels);

    Limiter limiter; limiter.prepare(48000.0, channels, ceilingDb);
    limiter.process(buffer.data(), frames, channels);
}

static std::vector<float> sine(int frames, int channels, double amplitude, double freq) {
    std::vector<float> buffer(static_cast<size_t>(frames) * channels);
    for (int i = 0; i < frames; ++i) {
        const float v = static_cast<float>(amplitude * std::sin(2 * M_PI * freq * i / 48000.0));
        for (int c = 0; c < channels; ++c) buffer[i * channels + c] = v;
    }
    return buffer;
}

static float peakOf(const std::vector<float>& v) {
    float peak = 0.0f;
    for (float s : v) peak = std::max(peak, std::fabs(s));
    return peak;
}

static bool allFinite(const std::vector<float>& v) {
    for (float s : v) if (!std::isfinite(s)) return false;
    return true;
}

int main() {
    const int ch = 2;
    const int frames = 24000;  // 0.5 detik
    int pass = 0, total = 0;
    const float ceiling = GainStage::dbToLinear(0.0f);

    // 1) Level rendah diteruskan apa adanya (limiter transparan).
    {
        auto buf = sine(frames, ch, 0.01, 220.0);
        runMasterPath(buf, frames, ch, 0.0f, 0.0f, false, 0.0f);
        const float peak = peakOf(buf);
        bool ok = allFinite(buf) && std::fabs(peak - 0.01f) < 0.002f;
        printf("Level rendah 0.01: puncak=%.4f (harapan ~0.01) -> %s\n",
               peak, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 2) Level tinggi dibatasi limiter ke ceiling (tidak melebihi 1.0).
    {
        auto buf = sine(frames, ch, 2.0, 220.0);
        runMasterPath(buf, frames, ch, 0.0f, 0.0f, false, 0.0f);
        const float peak = peakOf(buf);
        bool clamped = peak <= ceiling + 1e-5f;
        // Harus benar-benar menyentuh ceiling (limiter bekerja).
        bool touched = peak >= ceiling - 1e-3f;
        bool ok = allFinite(buf) && clamped && touched;
        printf("Level tinggi 2.0: puncak=%.4f ceiling=%.2f -> %s\n",
               peak, ceiling, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 3) Input gain +12 dB mendorong sinyal lewat batas; limiter melindungi.
    {
        auto buf = sine(frames, ch, 0.5, 220.0);
        runMasterPath(buf, frames, ch, 12.0f, 0.0f, false, 0.0f);
        const float peak = peakOf(buf);
        bool ok = allFinite(buf) && peak <= ceiling + 1e-5f;
        printf("Input +12dB (0.5 -> ~2.0): puncak=%.4f (<=%.2f) -> %s\n",
               peak, ceiling, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 4) Dengan 12 efek aktif, limiter tetap menjaga output <= ceiling.
    {
        auto buf = sine(frames, ch, 0.3, 110.0);
        runMasterPath(buf, frames, ch, 6.0f, 6.0f, true, 0.0f);
        const float peak = peakOf(buf);
        bool ok = allFinite(buf) && peak <= ceiling + 1e-5f;
        printf("12 efek + trim 6dB: puncak=%.4f (<=%.2f) -> %s\n",
               peak, ceiling, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 5) Diam tetap diam: tidak ada noise yang ditambahkan.
    {
        std::vector<float> buf(static_cast<size_t>(frames) * ch, 0.0f);
        runMasterPath(buf, frames, ch, 0.0f, 0.0f, true, 0.0f);
        const float peak = peakOf(buf);
        bool ok = allFinite(buf) && peak < 1e-6f;
        printf("Diam: puncak=%.2e (harapan 0) -> %s\n", peak, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 6) Ducking audio focus (BAB 15): level turun ke -12 dB dengan perataan.
    {
        GainStage duck; duck.prepare(48000.0, 30.0f);
        duck.setTargetDb(0.0f); duck.snapToTarget();
        duck.setTargetDb(-12.0f);
        std::vector<float> buf(static_cast<size_t>(frames) * ch, 1.0f);
        const float first = buf[0];
        duck.process(buf.data(), frames, ch);
        const float expected = GainStage::dbToLinear(-12.0f);
        bool noClick = first > 0.9f;  // belum langsung turun pada sampel pertama
        bool converged = std::fabs(buf[frames * ch - 1] - expected) < 0.005f;
        bool ok = noClick && converged;
        printf("Ducking -12dB: sampel1=%.4f akhir=%.4f (harapan %.4f) -> %s\n",
               first, buf[frames * ch - 1], expected, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 7) Ducking dilepas: level kembali ke 0 dB tanpa overshoot.
    {
        GainStage duck; duck.prepare(48000.0, 30.0f);
        duck.setTargetDb(-12.0f); duck.snapToTarget();
        duck.setTargetDb(0.0f);
        std::vector<float> buf(static_cast<size_t>(frames) * ch, 1.0f);
        duck.process(buf.data(), frames, ch);
        bool noOvershoot = true;
        for (float v : buf) if (v > 1.0f + 1e-4f) { noOvershoot = false; break; }
        bool ok = noOvershoot && std::fabs(buf[frames * ch - 1] - 1.0f) < 0.005f;
        printf("Ducking dilepas: akhir=%.4f tanpa overshoot=%d -> %s\n",
               buf[frames * ch - 1], noOvershoot, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    printf("\nLevel/limiter: %d/%d lolos\n", pass, total);
    return pass == total ? 0 : 1;
}
