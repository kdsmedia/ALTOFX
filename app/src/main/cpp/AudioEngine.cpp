#include "AudioEngine.h"

#include <chrono>

#include <android/log.h>

#define LOG_TAG "AltoAudio"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace alto {

namespace {
// Gitar bersifat mono; output stereo agar kompatibel dengan headphone/amp.
constexpr int32_t kInputChannels = 1;
constexpr int32_t kOutputChannels = 2;
constexpr int32_t kPreferredSampleRate = 48000;
}  // namespace

AudioEngine::AudioEngine() = default;

namespace {
void setError(std::mutex& lock, std::string& target, const std::string& message) {
    std::lock_guard<std::mutex> guard(lock);
    target = message;
}
}  // namespace

AudioEngine::~AudioEngine() {
    stopAudio();
}

std::string AudioEngine::version() const {
    return "ALTO GUITAR FX native engine 0.2 (BAB 4, Oboe full-duplex)";
}

oboe::Result AudioEngine::openStreams() {
    const int32_t outputDeviceId = outputDeviceId_.load();
    const int32_t inputDeviceId = inputDeviceId_.load();

    // Output dibuka lebih dulu (rekomendasi Oboe untuk hasil terbaik).
    oboe::AudioStreamBuilder outputBuilder;
    outputBuilder.setDirection(oboe::Direction::Output)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Exclusive)
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(kOutputChannels)
            ->setSampleRate(kPreferredSampleRate)
            ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
            ->setDataCallback(this)
            ->setErrorCallback(this);
    // -1 berarti perangkat default sistem.
    if (outputDeviceId >= 0) {
        outputBuilder.setDeviceId(outputDeviceId);
    }

    oboe::Result result = outputBuilder.openStream(outputStream_);
    if (result != oboe::Result::OK) {
        LOGE("Buka output gagal: %s", oboe::convertToText(result));
        outputStream_.reset();
        return result;
    }

    oboe::AudioStreamBuilder inputBuilder;
    inputBuilder.setDirection(oboe::Direction::Input)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Exclusive)
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(kInputChannels)
            // Unprocessed mencegah AGC/derau/noise-suppression bawaan yang
            // merusak sinyal gitar. Penting untuk mikrofon internal maupun
            // interface USB seperti iRig.
            ->setInputPreset(oboe::InputPreset::Unprocessed)
            ->setSampleRate(outputStream_->getSampleRate())
            ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
            // Buffer input dua kali kapasitas output agar tidak underrun.
            ->setBufferCapacityInFrames(outputStream_->getBufferCapacityInFrames() * 2)
            ->setErrorCallback(this);
    if (inputDeviceId >= 0) {
        inputBuilder.setDeviceId(inputDeviceId);
    }

    result = inputBuilder.openStream(inputStream_);
    if (result != oboe::Result::OK) {
        LOGE("Buka input gagal: %s", oboe::convertToText(result));
        inputStream_.reset();
        outputStream_->close();
        outputStream_.reset();
        return result;
    }

    setInputStream(inputStream_.get());
    setOutputStream(outputStream_.get());

    // Siapkan efek sesuai sample rate dan kanal output (BAB 7).
    effects_.prepare(outputStream_->getSampleRate(), outputStream_->getChannelCount());
    // Siapkan detektor pitch tuner (BAB 14) dengan sample rate stream.
    pitch_.prepare(outputStream_->getSampleRate());
    // Siapkan tahap gain trim input dan level output master (BAB 15).
    inputGain_.prepare(outputStream_->getSampleRate());
    outputGain_.prepare(outputStream_->getSampleRate());
    duckGain_.prepare(outputStream_->getSampleRate(), 30.0f);
    duckGain_.setTargetDb(ducked_.load() ? kDuckDb : 0.0f);
    // Siapkan pemantau performa callback (BAB 16).
    performance_.prepare(outputStream_->getSampleRate());

    sampleRate_.store(outputStream_->getSampleRate());
    channelCount_.store(outputStream_->getChannelCount());
    framesPerBurst_.store(outputStream_->getFramesPerBurst());
    LOGI("Stream siap: %d Hz, %d frame/burst, input ch=%d, output ch=%d",
         sampleRate_.load(), framesPerBurst_.load(), inputStream_->getChannelCount(),
         outputStream_->getChannelCount());
    return oboe::Result::OK;
}

std::string AudioEngine::startAudio() {
    std::lock_guard<std::mutex> guard(lock_);
    if (running_.load()) {
        return "Audio sudah berjalan";
    }
    clearLastError();
    oboe::Result result = openStreams();
    if (result != oboe::Result::OK) {
        const std::string message =
            std::string("Gagal membuka stream: ") + oboe::convertToText(result);
        setError(errorLock_, lastError_, message);
        return message;
    }
    result = FullDuplexStream::start();
    if (result != oboe::Result::OK) {
        LOGE("Start gagal: %s", oboe::convertToText(result));
        if (inputStream_) { inputStream_->close(); inputStream_.reset(); }
        if (outputStream_) { outputStream_->close(); outputStream_.reset(); }
        const std::string message =
            std::string("Gagal memulai stream: ") + oboe::convertToText(result);
        setError(errorLock_, lastError_, message);
        return message;
    }
    running_.store(true);
    return "Audio berjalan";
}

std::string AudioEngine::stopAudio() {
    std::lock_guard<std::mutex> guard(lock_);
    if (inputStream_ == nullptr && outputStream_ == nullptr) {
        running_.store(false);
        return "Audio sudah berhenti";
    }
    FullDuplexStream::stop();
    if (inputStream_) { inputStream_->close(); inputStream_.reset(); }
    if (outputStream_) { outputStream_->close(); outputStream_.reset(); }
    running_.store(false);
    sampleRate_.store(0);
    framesPerBurst_.store(0);
    channelCount_.store(0);
    return "Audio berhenti";
}

oboe::DataCallbackResult AudioEngine::onBothStreamsReady(const void *inputData, int numInputFrames,
                                                        void *outputData, int numOutputFrames) {
    // BAB 16: ukur lama eksekusi callback untuk memantau beban CPU real-time.
    const auto callbackStart = std::chrono::steady_clock::now();
    // BAB 6: jalur audio nyata input -> output dengan pengukuran level.
    // Efek DSP ditambahkan pada BAB 7.
    const auto *input = static_cast<const float *>(inputData);
    auto *output = static_cast<float *>(outputData);

    const int32_t inChannels = inputStream_ ? inputStream_->getChannelCount() : kInputChannels;
    const int32_t outChannels = outputStream_ ? outputStream_->getChannelCount() : kOutputChannels;

    // Jumlah frame yang bisa diproses bersama oleh kedua stream.
    int32_t frames = (numInputFrames < numOutputFrames) ? numInputFrames : numOutputFrames;

    // Salin input ke output lebih dulu, lalu terapkan trim gain input (BAB 15)
    // dengan perataan per-sampel agar perubahan tidak menimbulkan klik.
    for (int32_t frame = 0; frame < frames; ++frame) {
        const float sample = input[frame * inChannels];
        for (int32_t channel = 0; channel < outChannels; ++channel) {
            output[frame * outChannels + channel] = sample;
        }
    }
    inputGain_.process(output, frames, outChannels);

    // Deteksi clipping input setelah trim gain.
    float inputPeak = 0.0f;
    bool inputClip = false;
    for (int32_t frame = 0; frame < frames; ++frame) {
        const float sample = output[frame * outChannels];
        const float magnitude = (sample < 0.0f) ? -sample : sample;
        if (magnitude > inputPeak) {
            inputPeak = magnitude;
        }
        if (magnitude > 1.0f) {
            inputClip = true;
        }
    }
    inputClipping_.store(inputClip);

    // BAB 14: analisis pitch dari sinyal input mentah. Analisis hanya
    // dijalankan ketika tuner aktif (setActive), sehingga tidak membebani
    // CPU saat tuner tidak dibuka.
    pitch_.process(input, frames, inChannels);

    // BAB 7: jalankan rangkaian efek (Noise Gate, Compressor, ...) pada blok
    // output. Hanya efek aktif yang diproses; tanpa alokasi atau lock.
    effects_.process(output, frames, outChannels);

    // Level output master + limiter (BAB 15). Limiter membatasi puncak ke
    // batas yang ditetapkan sehingga mencegah clipping keras di perangkat.
    outputGain_.process(output, frames, outChannels);
    // Ducking audio focus (BAB 15): level diturunkan sementara bila aplikasi
    // lain butuh perhatian, dengan perataan agar tidak menimbulkan klik.
    duckGain_.process(output, frames, outChannels);
    const float ceiling = GainStage::dbToLinear(limiterCeilingDb_.load());
    float outputPeak = 0.0f;
    bool outputClip = false;
    for (int32_t frame = 0; frame < frames; ++frame) {
        for (int32_t channel = 0; channel < outChannels; ++channel) {
            float& sample = output[frame * outChannels + channel];
            if (sample > ceiling) {
                sample = ceiling;
                outputClip = true;
            } else if (sample < -ceiling) {
                sample = -ceiling;
                outputClip = true;
            }
            const float magnitude = (sample < 0.0f) ? -sample : sample;
            if (magnitude > outputPeak) {
                outputPeak = magnitude;
            }
        }
    }
    outputClipping_.store(outputClip);

    inputLevel_.store(inputPeak);
    outputLevel_.store(outputPeak);
    callbackCount_.fetch_add(1);
    framesProcessed_.fetch_add(frames);

    // BAB 16: catat durasi callback untuk statistik performa.
    const auto callbackEnd = std::chrono::steady_clock::now();
    const double micros =
        std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(
            callbackEnd - callbackStart).count();
    performance_.record(micros, frames);

    return oboe::DataCallbackResult::Continue;
}

float AudioEngine::estimatedLatencyMs() const {
    const double rate = static_cast<double>(sampleRate_.load());
    if (rate <= 0.0) {
        return 0.0f;
    }
    // Latensi per arah kira-kira sebesar kapasitas buffer stream. Round-trip
    // = input + output.
    const int32_t capacity = bufferCapacityFrames();
    if (capacity <= 0) {
        return 0.0f;
    }
    return static_cast<float>((2.0 * capacity) / rate * 1000.0);
}

int32_t AudioEngine::bufferCapacityFrames() const {
    if (outputStream_ != nullptr) {
        return outputStream_->getBufferCapacityInFrames();
    }
    return 0;
}

int64_t AudioEngine::xrunCount() const {
    // Oboe melaporkan xrun lewat stream; baca dari stream output bila ada.
    if (outputStream_ != nullptr) {
        auto result = outputStream_->getXRunCount();
        if (result) {
            return result.value();
        }
    }
    return 0;
}

void AudioEngine::onErrorAfterClose(oboe::AudioStream * /*stream*/, oboe::Result error) {
    // Perangkat berubah (mis. iRig dicabut/dicolok) atau koneksi terputus.
    if (running_.load() && error == oboe::Result::ErrorDisconnected) {
        LOGI("Perangkat terputus, mencoba membuka ulang stream");
        setError(errorLock_, lastError_,
                 "Perangkat audio terputus; mencoba menyambung ulang.");
        std::lock_guard<std::mutex> guard(lock_);
        if (inputStream_) { inputStream_->close(); inputStream_.reset(); }
        if (outputStream_) { outputStream_->close(); outputStream_.reset(); }
        if (openStreams() == oboe::Result::OK) {
            FullDuplexStream::start();
            clearLastError();
        } else {
            running_.store(false);
            setError(errorLock_, lastError_,
                     "Perangkat audio terputus dan gagal dibuka ulang.");
        }
    }
}

}  // namespace alto
