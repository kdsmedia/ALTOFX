#include "AudioEngine.h"
#include "LatencyMath.h"

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

oboe::Result AudioEngine::openOutputStream(oboe::AudioStreamBuilder& builder) {
    // Strategi berlapis: coba Exclusive + LowLatency (latensi terbaik), lalu
    // turun ke Shared bila perangkat menolak. Tanpa fallback ini, banyak
    // perangkat menolak Exclusive dan aplikasi tampak "tidak berbunyi".
    // Setelah percobaan pertama gagal, DeviceId spesifik dibuang agar
    // percobaan berikutnya memakai perangkat default sistem.
    int32_t deviceId = outputDeviceId_.load();
    if (deviceId < 0) {
        deviceId = oboe::kUnspecified;
    }

    struct Attempt {
        oboe::SharingMode sharing;
        oboe::PerformanceMode performance;
        int32_t sampleRate;  // 0 = biarkan perangkat memilih (kUnspecified).
    };
    const Attempt attempts[] = {
            {oboe::SharingMode::Exclusive, oboe::PerformanceMode::LowLatency,
             preferredSampleRate_.load()},
            {oboe::SharingMode::Shared, oboe::PerformanceMode::LowLatency,
             preferredSampleRate_.load()},
            {oboe::SharingMode::Shared, oboe::PerformanceMode::None,
             preferredSampleRate_.load()},
            // Upaya terakhir: biarkan perangkat memilih sample rate-nya sendiri.
            {oboe::SharingMode::Shared, oboe::PerformanceMode::None, oboe::kUnspecified},
    };

    oboe::Result lastResult = oboe::Result::ErrorInternal;
    for (const auto& attempt : attempts) {
        builder.setDirection(oboe::Direction::Output)
                ->setSharingMode(attempt.sharing)
                ->setPerformanceMode(attempt.performance)
                ->setFormat(oboe::AudioFormat::Float)
                // Izinkan konversi format/kanal agar tetap dapat jalur
                // low-latency pada perangkat yang hanya mendukung I16/mono.
                ->setFormatConversionAllowed(true)
                ->setChannelConversionAllowed(true)
                ->setChannelCount(kOutputChannels)
                ->setSampleRate(attempt.sampleRate)
                ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
                ->setDeviceId(deviceId)
                ->setDataCallback(this)
                ->setErrorCallback(this);
        lastResult = builder.openStream(outputStream_);
        if (lastResult == oboe::Result::OK) {
            return lastResult;
        }
        LOGE("Buka output gagal (sharing=%d, perf=%d, rate=%d): %s",
             static_cast<int>(attempt.sharing), static_cast<int>(attempt.performance),
             attempt.sampleRate, oboe::convertToText(lastResult));
        outputStream_.reset();
        // Perangkat spesifik tidak ada lagi: pakai perangkat default sistem.
        deviceId = oboe::kUnspecified;
    }
    return lastResult;
}

oboe::Result AudioEngine::openInputStream(oboe::AudioStreamBuilder& builder) {
    int32_t deviceId = inputDeviceId_.load();
    if (deviceId < 0) {
        deviceId = oboe::kUnspecified;
    }
    const bool headset = headsetInput_.load();
    const int32_t outputSampleRate = outputStream_ ? outputStream_->getSampleRate() : 0;
    const int32_t outputCapacity =
            outputStream_ ? outputStream_->getBufferCapacityInFrames() : 0;

    struct Attempt {
        oboe::SharingMode sharing;
        oboe::PerformanceMode performance;
        oboe::InputPreset preset;
        int32_t channels;
    };
    // Urutan: jalur terbaik lebih dulu. Bila input headset (iRig di jack
    // headset), coba VoiceRecognition lebih awal karena pada banyak perangkat
    // preset itulah yang membuka mikrofon headset (TRRS) dengan benar. Untuk
    // input lain, Unprocessed memberi sinyal gitar apa adanya (tanpa AGC).
    // Bila semua gagal, turun ke Shared/Generic lalu Unprocessed sebagai
    // upaya terakhir.
    const Attempt attempts[] = {
            {oboe::SharingMode::Exclusive, oboe::PerformanceMode::LowLatency,
             headset ? oboe::InputPreset::VoiceRecognition : oboe::InputPreset::Unprocessed,
             kInputChannels},
            {oboe::SharingMode::Exclusive, oboe::PerformanceMode::LowLatency,
             oboe::InputPreset::Generic, kInputChannels},
            {oboe::SharingMode::Exclusive, oboe::PerformanceMode::LowLatency,
             oboe::InputPreset::Unprocessed, kInputChannels},
            {oboe::SharingMode::Shared, oboe::PerformanceMode::LowLatency,
             oboe::InputPreset::Generic, kInputChannels},
            {oboe::SharingMode::Shared, oboe::PerformanceMode::None,
             oboe::InputPreset::Generic, kInputChannels},
    };

    oboe::Result lastResult = oboe::Result::ErrorInternal;
    for (const auto& attempt : attempts) {
        builder.setDirection(oboe::Direction::Input)
                ->setSharingMode(attempt.sharing)
                ->setPerformanceMode(attempt.performance)
                ->setFormat(oboe::AudioFormat::Float)
                ->setFormatConversionAllowed(true)
                ->setChannelConversionAllowed(true)
                ->setInputPreset(attempt.preset)
                ->setChannelCount(attempt.channels)
                ->setSampleRate(outputSampleRate)
                ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
                ->setDeviceId(deviceId)
                ->setErrorCallback(this);
        // Buffer input dua kali kapasitas output agar tidak underrun.
        if (outputCapacity > 0) {
            builder.setBufferCapacityInFrames(outputCapacity * 2);
        }
        lastResult = builder.openStream(inputStream_);
        if (lastResult == oboe::Result::OK) {
            return lastResult;
        }
        LOGE("Buka input gagal (sharing=%d, preset=%d): %s",
             static_cast<int>(attempt.sharing), static_cast<int>(attempt.preset),
             oboe::convertToText(lastResult));
        inputStream_.reset();
        // Perangkat spesifik tidak ada lagi: pakai perangkat default sistem.
        deviceId = oboe::kUnspecified;
    }
    return lastResult;
}

oboe::Result AudioEngine::openStreams() {
    std::string packageName;
    {
        std::lock_guard<std::mutex> guard(errorLock_);
        packageName = packageName_;
    }
    // Output dibuka lebih dulu (rekomendasi Oboe untuk hasil terbaik).
    oboe::AudioStreamBuilder outputBuilder;
    if (!packageName.empty()) {
        outputBuilder.setPackageName(packageName);
    }
    oboe::Result result = openOutputStream(outputBuilder);
    if (result != oboe::Result::OK) {
        outputStream_.reset();
        return result;
    }

    oboe::AudioStreamBuilder inputBuilder;
    if (!packageName.empty()) {
        // Penting untuk stream input pada Android 12+ (pemeriksaan izin).
        inputBuilder.setPackageName(packageName);
    }
    result = openInputStream(inputBuilder);
    if (result != oboe::Result::OK) {
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
    // Peredam dengung/grounding pada jalur input (notch 50/60 Hz + harmonisa).
    humFilter_.setSampleRate(outputStream_->getSampleRate());
    humFilter_.prepare(outputStream_->getSampleRate(), outputStream_->getChannelCount());
    humFilter_.setHumFrequency(humHz_.load());
    // Penekan feedback otomatis (anti-howling) pada jalur input.
    antiFeedback_.prepare(outputStream_->getSampleRate(), outputStream_->getChannelCount());
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

    // Pertahanan: bila tidak ada data (mis. input belum siap), jangan sentuh
    // pointer null. Output sudah dikosongkan (silence) oleh FullDuplexStream.
    if (input == nullptr || output == nullptr) {
        return oboe::DataCallbackResult::Continue;
    }

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

    // Peredam dengung/grounding (notch 50/60 Hz + harmonisa + high-pass).
    // Diletakkan sebelum efek saturasi agar dengung tidak ikut diperkuat.
    if (humEnabled_.load()) {
        // Perbarui koefisien notch di thread audio bila frekuensi berubah dari
        // UI (objek DSP tidak boleh disentuh dari dua thread).
        if (humPending_.exchange(false)) {
            humFilter_.setHumFrequency(humHz_.load());
        }
        humFilter_.process(output, frames, outChannels);
    }

    // Penekan feedback otomatis: cari nada yang menonjol & bertahan (howling)
    // lalu pasang notch sempit agar suara tetap jernih.
    if (antiFeedbackEnabled_.load()) {
        antiFeedback_.process(output, frames, outChannels);
    }

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

float AudioEngine::measuredLatencyMs() const {
    if (outputStream_ == nullptr) {
        return -1.0f;
    }
    const double rate = static_cast<double>(sampleRate_.load());
    if (rate <= 0.0) {
        return -1.0f;
    }
    int64_t outputPos = 0;
    int64_t outputTime = 0;
    const auto outputResult = outputStream_->getTimestamp(
            CLOCK_MONOTONIC, &outputPos, &outputTime);
    if (outputResult != oboe::Result::OK) {
        return -1.0f;
    }
    int64_t inputPos = 0;
    int64_t inputTime = 0;
    if (inputStream_ == nullptr) {
        return -1.0f;
    }
    const auto inputResult = inputStream_->getTimestamp(
            CLOCK_MONOTONIC, &inputPos, &inputTime);
    if (inputResult != oboe::Result::OK) {
        return -1.0f;
    }
    const double ms = roundTripLatencyMs(
            rate,
            outputStream_->getFramesWritten(), outputPos,
            inputStream_->getFramesRead(), inputPos);
    return (ms < 0.0) ? -1.0f : static_cast<float>(ms);
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
    if (error != oboe::Result::ErrorDisconnected || !running_.load()) {
        return;
    }
    LOGI("Perangkat terputus, mencoba membuka ulang stream");
    setError(errorLock_, lastError_,
             "Perangkat audio terputus; mencoba menyambung ulang.");

    // Pakai try_lock: bila thread lain sedang start/stop, biarkan thread itu
    // yang menangani. Menunggu di sini bisa mengunci callback error.
    std::unique_lock<std::mutex> guard(lock_, std::try_to_lock);
    if (!guard.owns_lock()) {
        return;
    }
    // Stream sudah ditutup oleh Oboe; lepaskan referensi kita.
    if (inputStream_) { inputStream_->close(); inputStream_.reset(); }
    if (outputStream_) { outputStream_->close(); outputStream_.reset(); }
    running_.store(false);
    if (openStreams() == oboe::Result::OK && FullDuplexStream::start() == oboe::Result::OK) {
        running_.store(true);
        clearLastError();
    } else {
        if (inputStream_) { inputStream_->close(); inputStream_.reset(); }
        if (outputStream_) { outputStream_->close(); outputStream_.reset(); }
        setError(errorLock_, lastError_,
                 "Perangkat audio terputus dan gagal dibuka ulang.");
    }
}

}  // namespace alto
