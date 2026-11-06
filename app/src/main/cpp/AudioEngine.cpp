#include "AudioEngine.h"

namespace alto {

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() = default;

std::string AudioEngine::version() const {
    return "ALTO GUITAR FX native engine 0.1 (BAB 3)";
}

}  // namespace alto
