#include "Effects.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

using namespace alto;

// Katalog efek sesuai blueprint bagian 7: jumlah parameter dan nama harus
// sejalan antara DSP (C++) dan katalog UI (EffectType.kt). Uji ini menjaga
// agar penambahan/penghapusan parameter tidak lewat begitu saja.
struct CatalogEntry {
    EffectType type;
    const char* name;
    int paramCount;
};

static const CatalogEntry kCatalog[] = {
    {EffectType::NoiseGate, "Noise Gate", 3},
    {EffectType::Compressor, "Compressor", 5},
    {EffectType::Overdrive, "Overdrive", 3},
    {EffectType::Distortion, "Distortion", 3},
    {EffectType::Fuzz, "Fuzz", 3},
    {EffectType::Amp, "Amp", 7},
    {EffectType::Equalizer, "Equalizer", 3},
    {EffectType::Chorus, "Chorus", 3},
    {EffectType::Flanger, "Flanger", 4},
    {EffectType::Phaser, "Phaser", 4},
    {EffectType::Tremolo, "Tremolo", 2},
    {EffectType::Delay, "Delay", 3},
    {EffectType::Reverb, "Reverb", 4},
};

// Petikan gitar: kuat lalu meluruh, cukup untuk memicu efek bergantung level.
static std::vector<float> pluck(double sr, int frames, int channels) {
    std::vector<float> buffer(static_cast<size_t>(frames) * channels);
    for (int i = 0; i < frames; ++i) {
        const double t = i / sr;
        const double env = std::exp(-6.0 * t);
        const double v = std::sin(2 * M_PI * 220.0 * t) + 0.3 * std::sin(2 * M_PI * 440.0 * t);
        const float s = static_cast<float>(v * env * 0.5);
        for (int c = 0; c < channels; ++c) buffer[i * channels + c] = s;
    }
    return buffer;
}

static void configureStrong(EffectChain& chain, EffectType type) {
    switch (type) {
        case EffectType::NoiseGate:
            chain.setParam(type, 0, -30.0f); chain.setParam(type, 1, 50.0f);
            chain.setParam(type, 2, 2.0f);
            break;
        case EffectType::Compressor:
            chain.setParam(type, 0, -30.0f); chain.setParam(type, 1, 8.0f);
            chain.setParam(type, 2, 5.0f); chain.setParam(type, 3, 80.0f);
            chain.setParam(type, 4, 12.0f);
            break;
        case EffectType::Overdrive:
        case EffectType::Distortion:
        case EffectType::Fuzz:
            chain.setParam(type, 0, 0.9f); chain.setParam(type, 1, 0.6f);
            chain.setParam(type, 2, 0.9f);
            break;
        case EffectType::Amp:
            chain.setParam(type, 0, 2.0f);   // voicing Lead
            chain.setParam(type, 1, 18.0f);  // gain
            chain.setParam(type, 2, 8.0f);   // bass
            chain.setParam(type, 3, 6.0f);   // mid
            chain.setParam(type, 4, 8.0f);   // treble
            chain.setParam(type, 5, 6.0f);   // presence
            chain.setParam(type, 6, 6.0f);   // master
            break;
        case EffectType::Equalizer:
            chain.setParam(type, 0, 10.0f); chain.setParam(type, 1, -10.0f);
            chain.setParam(type, 2, 10.0f);
            break;
        case EffectType::Chorus:
            chain.setParam(type, 0, 2.0f); chain.setParam(type, 1, 0.9f);
            chain.setParam(type, 2, 0.9f);
            break;
        case EffectType::Flanger:
        case EffectType::Phaser:
            chain.setParam(type, 0, 1.0f); chain.setParam(type, 1, 0.9f);
            chain.setParam(type, 2, 0.8f); chain.setParam(type, 3, 0.9f);
            break;
        case EffectType::Tremolo:
            chain.setParam(type, 0, 8.0f); chain.setParam(type, 1, 0.9f);
            break;
        case EffectType::Delay:
            chain.setParam(type, 0, 200.0f); chain.setParam(type, 1, 0.7f);
            chain.setParam(type, 2, 0.8f);
            break;
        case EffectType::Reverb:
            chain.setParam(type, 0, 0.9f); chain.setParam(type, 1, 0.9f);
            chain.setParam(type, 2, 0.1f); chain.setParam(type, 3, 0.8f);
            break;
        default: break;
    }
}

static double rmsDiff(const std::vector<float>& a, const std::vector<float>& b) {
    double sum = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        const double d = a[i] - b[i];
        sum += d * d;
    }
    return std::sqrt(sum / static_cast<double>(a.size()));
}

#ifdef KT_EFFECTTYPE_PATH
// Membaca katalog UI (EffectType.kt) dan memastikan id, label, serta jumlah
// parameter sejalan dengan DSP. Ini menjaga agar katalog efek tidak lepas
// sinkron antara Kotlin dan C++ (blueprint bagian 7 & 12).
struct KotlinEntry {
    int id;
    std::string label;
    int paramCount;
};

static std::vector<KotlinEntry> parseKotlinCatalog(const char* path, bool& ok) {
    std::ifstream file(path);
    std::vector<KotlinEntry> entries;
    ok = false;
    if (!file) {
        printf("  tidak dapat membuka %s\n", path);
        return entries;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    const std::string text = ss.str();
    ok = true;

    size_t pos = 0;
    while (true) {
        const size_t idPos = text.find("id = ", pos);
        if (idPos == std::string::npos) break;
        const size_t idEnd = text.find(',', idPos);
        const size_t labelPos = text.find("label = \"", idEnd);
        if (idEnd == std::string::npos || labelPos == std::string::npos) break;
        const size_t labelStart = labelPos + std::strlen("label = \"");
        const size_t labelEnd = text.find('"', labelStart);
        const size_t paramsPos = text.find("params = listOf(", labelEnd);
        if (labelEnd == std::string::npos || paramsPos == std::string::npos) break;
        const size_t paramsEnd = text.find('\n' + std::string(8, ' ') + ')', paramsPos);
        if (paramsEnd == std::string::npos) break;

        KotlinEntry entry;
        entry.id = std::stoi(text.substr(idPos + 5, idEnd - idPos - 5));
        entry.label = text.substr(labelStart, labelEnd - labelStart);
        entry.paramCount = 0;
        size_t p = paramsPos;
        while ((p = text.find("ParamSpec(", p)) != std::string::npos && p < paramsEnd) {
            ++entry.paramCount;
            p += 10;
        }
        entries.push_back(entry);
        pos = paramsEnd;
    }
    return entries;
}
#endif

int main() {
    const double sr = 48000.0;
    const int ch = 2;
    int pass = 0, total = 0;

    // 1) Kontrak katalog: nama & jumlah parameter tiap efek.
    {
        EffectChain chain;
        chain.prepare(sr, ch);
        bool ok = true;
        for (const auto& entry : kCatalog) {
            const bool nameOk = std::strcmp(chain.name(entry.type), entry.name) == 0;
            const bool countOk = chain.paramCount(entry.type) == entry.paramCount;
            if (!nameOk || !countOk) {
                printf("  %s: nama=%s (harus %s), param=%d (harus %d)\n",
                       entry.name, chain.name(entry.type), entry.name,
                       chain.paramCount(entry.type), entry.paramCount);
                ok = false;
            }
        }
        printf("Katalog: %zu efek, nama & jumlah parameter sesuai blueprint -> %s\n",
               sizeof(kCatalog) / sizeof(kCatalog[0]), ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 2) Setiap parameter dapat disetel dan dibaca kembali (dipakai UI/preset).
    {
        EffectChain chain;
        chain.prepare(sr, ch);
        bool ok = true;
        for (const auto& entry : kCatalog) {
            for (int i = 0; i < entry.paramCount; ++i) {
                const float value = 0.25f + 0.05f * static_cast<float>(i);
                chain.setParam(entry.type, i, value);
                if (std::fabs(chain.param(entry.type, i) - value) > 1e-6f) {
                    printf("  %s param %d tidak bulat balik\n", entry.name, i);
                    ok = false;
                }
            }
        }
        printf("Katalog: set/get seluruh parameter -> %s\n", ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 3) Tiap efek benar-benar mengubah sinyal saat aktif (bukan dummy).
    {
        EffectChain chain;
        chain.prepare(sr, ch);
        bool ok = true;
        for (const auto& entry : kCatalog) {
            for (int i = 0; i < EffectChain::kEffectCount; ++i) {
                chain.setEnabled(static_cast<EffectType>(i), false);
            }
            chain.setEnabled(entry.type, true);
            configureStrong(chain, entry.type);
            auto buf = pluck(sr, 48000, ch);
            const auto dry = buf;
            chain.process(buf.data(), 48000, ch);
            const double diff = rmsDiff(buf, dry);
            if (!(diff > 1e-3)) {
                printf("  %s tidak mengubah sinyal (diff=%.2e)\n", entry.name, diff);
                ok = false;
            }
        }
        printf("Katalog: setiap efek mengubah sinyal -> %s\n", ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 4) Reset menyetel ulang keadaan internal setiap efek (blueprint bagian 7).
    {
        EffectChain chain;
        chain.prepare(sr, ch);
        for (int i = 0; i < EffectChain::kEffectCount; ++i) {
            const auto type = static_cast<EffectType>(i);
            chain.setEnabled(type, true);
            configureStrong(chain, type);
        }
        auto buf = pluck(sr, 48000, ch);
        chain.process(buf.data(), 48000, ch);
        chain.reset();
        const int frames = 2048;
        std::vector<float> silence(static_cast<size_t>(frames) * ch, 0.0f);
        chain.process(silence.data(), frames, ch);
        float peak = 0.0f;
        for (float s : silence) peak = std::max(peak, std::fabs(s));
        const bool ok = peak < 1e-4f;
        printf("Katalog: reset seluruh efek -> puncak=%.2e %s\n",
               peak, ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

    // 5) Katalog bekerja pada sample rate berbeda (mis. 96 kHz).
    {
        EffectChain chain;
        chain.prepare(96000.0, ch);
        for (int i = 0; i < EffectChain::kEffectCount; ++i) {
            const auto type = static_cast<EffectType>(i);
            chain.setEnabled(type, true);
            configureStrong(chain, type);
        }
        auto buf = pluck(96000.0, 96000, ch);
        chain.process(buf.data(), 96000, ch);
        bool ok = true;
        for (float s : buf) {
            if (!std::isfinite(s) || std::fabs(s) >= 10.0f) { ok = false; break; }
        }
        printf("Katalog @96 kHz: seluruh efek stabil -> %s\n", ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }

#ifdef KT_EFFECTTYPE_PATH
    // 6) Sinkronisasi katalog Kotlin (UI) dengan DSP C++.
    {
        bool parsed = false;
        const auto entries = parseKotlinCatalog(KT_EFFECTTYPE_PATH, parsed);
        bool ok = parsed && entries.size() == (sizeof(kCatalog) / sizeof(kCatalog[0]));
        if (ok) {
            for (size_t i = 0; i < entries.size(); ++i) {
                const auto& kt = entries[i];
                const auto& cpp = kCatalog[i];
                if (kt.id != static_cast<int>(cpp.type) ||
                    kt.label != cpp.name ||
                    kt.paramCount != cpp.paramCount) {
                    printf("  %s: Kotlin(id=%d,\"%s\",%d) vs C++(id=%d,\"%s\",%d)\n",
                           cpp.name, kt.id, kt.label.c_str(), kt.paramCount,
                           static_cast<int>(cpp.type), cpp.name, cpp.paramCount);
                    ok = false;
                }
            }
        } else {
            printf("  jumlah entri Kotlin=%zu, C++=%zu (gagal parse=%d)\n",
                   entries.size(), sizeof(kCatalog) / sizeof(kCatalog[0]),
                   parsed ? 0 : 1);
        }
        printf("Katalog: EffectType.kt sinkron dengan DSP C++ -> %s\n",
               ok ? "OK" : "GAGAL");
        pass += ok; total++;
    }
#endif

    printf("\nKatalog efek (BAB 7): %d/%d lolos\n", pass, total);
    return pass == total ? 0 : 1;
}
