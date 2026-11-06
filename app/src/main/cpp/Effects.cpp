#include "Effects.h"

#include "Compressor.h"
#include "DriveEffect.h"
#include "NoiseGate.h"

namespace alto {

EffectChain::EffectChain() {
    // BAB 7: efek dinamika.
    effects_[static_cast<size_t>(EffectType::NoiseGate)] = std::make_unique<NoiseGate>();
    effects_[static_cast<size_t>(EffectType::Compressor)] = std::make_unique<Compressor>();
    // BAB 8: efek saturasi/gain.
    effects_[static_cast<size_t>(EffectType::Overdrive)] = std::make_unique<DriveEffect>(
            "Overdrive", DriveEffect::Shape::SoftTanh, 30.0f);
    effects_[static_cast<size_t>(EffectType::Distortion)] = std::make_unique<DriveEffect>(
            "Distortion", DriveEffect::Shape::HardClip, 120.0f);
    effects_[static_cast<size_t>(EffectType::Fuzz)] = std::make_unique<DriveEffect>(
            "Fuzz", DriveEffect::Shape::Fuzz, 400.0f);
}

void EffectChain::prepare(double sampleRate, int32_t channels) {
    for (auto& effect : effects_) {
        if (effect != nullptr) {
            effect->prepare(sampleRate, channels);
        }
    }
}

void EffectChain::process(float* buffer, int32_t frames, int32_t channels) {
    for (auto& effect : effects_) {
        if (effect != nullptr && effect->isEnabled()) {
            effect->process(buffer, frames, channels);
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
