#include "Effects.h"

#include <algorithm>

#include "AmpModel.h"
#include "Chorus.h"
#include "Compressor.h"
#include "Delay.h"
#include "DriveEffect.h"
#include "Equalizer.h"
#include "Flanger.h"
#include "NoiseGate.h"
#include "Phaser.h"
#include "Reverb.h"
#include "Tremolo.h"

namespace alto {

EffectChain::EffectChain() {
    // BAB 7: efek dinamika.
    effects_[static_cast<size_t>(EffectType::NoiseGate)] = std::make_unique<NoiseGate>();
    effects_[static_cast<size_t>(EffectType::Compressor)] = std::make_unique<Compressor>();
    // BAB 8: efek saturasi/gain. Skala drive dinaikkan agar sinyal tetap
    // terjenuh selama nada meluruh (khas gitar) — bukan hanya pada serangan
    // awal. Tanpa ini, distorsi "memudar" kembali menjadi suara bersih.
    effects_[static_cast<size_t>(EffectType::Overdrive)] = std::make_unique<DriveEffect>(
            "Overdrive", DriveEffect::Shape::SoftTanh, 200.0f);
    effects_[static_cast<size_t>(EffectType::Distortion)] = std::make_unique<DriveEffect>(
            "Distortion", DriveEffect::Shape::HardClip, 800.0f);
    effects_[static_cast<size_t>(EffectType::Fuzz)] = std::make_unique<DriveEffect>(
            "Fuzz", DriveEffect::Shape::Fuzz, 2000.0f);
    // Model amplifier bergaya Tonebridge (preamp + tone stack + kabinet).
    effects_[static_cast<size_t>(EffectType::Amp)] = std::make_unique<AmpModel>();
    // BAB 9: equalizer 3-band.
    effects_[static_cast<size_t>(EffectType::Equalizer)] = std::make_unique<Equalizer>();
    // BAB 10: efek modulasi.
    effects_[static_cast<size_t>(EffectType::Chorus)] = std::make_unique<Chorus>();
    effects_[static_cast<size_t>(EffectType::Flanger)] = std::make_unique<Flanger>();
    effects_[static_cast<size_t>(EffectType::Phaser)] = std::make_unique<Phaser>();
    effects_[static_cast<size_t>(EffectType::Tremolo)] = std::make_unique<Tremolo>();
    // BAB 11: efek ruang/waktu.
    effects_[static_cast<size_t>(EffectType::Delay)] = std::make_unique<Delay>();
    effects_[static_cast<size_t>(EffectType::Reverb)] = std::make_unique<Reverb>();

    resetOrder();
}

uint64_t EffectChain::encode(const std::array<EffectType, kEffectCount>& order) {
    uint64_t code = 0;
    for (int i = 0; i < kEffectCount; ++i) {
        code |= static_cast<uint64_t>(static_cast<uint8_t>(order[static_cast<size_t>(i)]))
                << (4 * i);
    }
    return code;
}

std::array<EffectType, EffectChain::kEffectCount> EffectChain::decode(uint64_t code) {
    std::array<EffectType, kEffectCount> order{};
    for (int i = 0; i < kEffectCount; ++i) {
        const auto value = static_cast<uint8_t>((code >> (4 * i)) & 0xF);
        // Nilai di luar rentang diperlakukan sebagai efek pertama (aman).
        order[static_cast<size_t>(i)] = (value < static_cast<uint8_t>(EffectType::Count))
                                                ? static_cast<EffectType>(value)
                                                : EffectType::NoiseGate;
    }
    return order;
}

std::array<EffectType, EffectChain::kEffectCount> EffectChain::order() const {
    return decode(orderCode_.load());
}

void EffectChain::setOrder(const std::array<EffectType, kEffectCount>& order) {
    orderCode_.store(encode(order));
}

void EffectChain::moveEffect(EffectType type, int delta) {
    auto current = order();
    int from = -1;
    for (int i = 0; i < kEffectCount; ++i) {
        if (current[static_cast<size_t>(i)] == type) {
            from = i;
            break;
        }
    }
    if (from < 0) {
        return;
    }
    const int to = from + delta;
    if (to < 0 || to >= kEffectCount) {
        return;
    }
    std::swap(current[static_cast<size_t>(from)], current[static_cast<size_t>(to)]);
    orderCode_.store(encode(current));
}

void EffectChain::reset() {
    for (auto& effect : effects_) {
        if (effect) {
            effect->reset();
        }
    }
}

void EffectChain::resetOrder() {
    std::array<EffectType, kEffectCount> order{};
    for (int i = 0; i < kEffectCount; ++i) {
        order[static_cast<size_t>(i)] = static_cast<EffectType>(i);
    }
    orderCode_.store(encode(order));
}

void EffectChain::prepare(double sampleRate, int32_t channels) {
    for (auto& effect : effects_) {
        if (effect != nullptr) {
            effect->prepare(sampleRate, channels);
        }
    }
}

void EffectChain::process(float* buffer, int32_t frames, int32_t channels) {
    // Bypass keseluruhan: sinyal dibiarkan apa adanya tanpa efek.
    if (globalBypass_.load()) {
        return;
    }
    const auto current = decode(orderCode_.load());
    for (const auto type : current) {
        Effect* e = effect(type);
        if (e != nullptr && e->isEnabled()) {
            e->process(buffer, frames, channels);
        }
    }
}

Effect* EffectChain::effect(EffectType type) const {
    const auto index = static_cast<size_t>(type);
    if (index < effects_.size()) {
        return effects_[index].get();
    }
    return nullptr;
}

void EffectChain::setEnabled(EffectType type, bool enabled) {
    if (Effect* e = effect(type)) {
        e->setEnabled(enabled);
    }
}

bool EffectChain::isEnabled(EffectType type) const {
    if (Effect* e = effect(type)) {
        return e->isEnabled();
    }
    return false;
}

void EffectChain::setParam(EffectType type, int index, float value) {
    if (Effect* e = effect(type)) {
        e->setParam(index, value);
    }
}

float EffectChain::param(EffectType type, int index) const {
    if (Effect* e = effect(type)) {
        return e->param(index);
    }
    return 0.0f;
}

int EffectChain::paramCount(EffectType type) const {
    if (Effect* e = effect(type)) {
        return e->paramCount();
    }
    return 0;
}

const char* EffectChain::name(EffectType type) const {
    if (Effect* e = effect(type)) {
        return e->name();
    }
    return "";
}

}  // namespace alto
