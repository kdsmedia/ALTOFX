#ifndef ALTOGUITARFX_AUDIOENGINE_H
#define ALTOGUITARFX_AUDIOENGINE_H

#include <oboe/FullDuplexStream.h>
#include <oboe/Oboe.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#include "Effects.h"
#include "GainStage.h"
#include "PerformanceMonitor.h"
#include "PitchDetector.h"

namespace alto {

/**
 * Mesin audio real-time (BAB 4).
 *
 * Memakai dua stream Oboe (input + output) yang disinkronkan oleh
 * oboe::FullDuplexStream. Ini adalah pola resmi Oboe untuk pemrosesan gitar
 * real-time: input dari mikrofon / interface USB seperti iRig, output ke
 * headphone / speaker.
 *
 * Pada BAB 4 audio diteruskan apa adanya (dry passthrough) sebagai fondasi;
 * efek DSP ditambahkan mulai BAB 7. Tidak ada simulasi atau data dummy: stream
 * dibuka dari perangkat audio asli dan benar-benar mengalirkan sinyal.
 *
 * Aturan real-time: callback tidak melakukan alokasi memori, I/O disk, lock
 * blocking, atau logging.
 */
class AudioEngine : public oboe::FullDuplexStream, public oboe::AudioStreamErrorCallback {
public:
    AudioEngine();
    ~AudioEngine() override;

    /** Membuka stream lalu memulainya. Mengembalikan pesan status. */
    std::string startAudio();

    /** Menghentikan dan menutup stream. Mengembalikan pesan status. */
    std::string stopAudio();

    bool isRunning() const { return running_.load(); }

    int32_t sampleRate() const { return sampleRate_.load(); }

    int32_t framesPerBurst() const { return framesPerBurst_.load(); }

    // ---- Pemantauan level & diagnostik (BAB 6) ----

    /** Level puncak (peak) blok input terakhir, 0..1 linier. */
    float inputLevel() const { return inputLevel_.load(); }

    /** Level puncak (peak) blok output terakhir, 0..1 linier. */
    float outputLevel() const { return outputLevel_.load(); }

    /** Jumlah callback audio yang sudah dijalankan sejak stream dibuka. */
    int64_t callbackCount() const { return callbackCount_.load(); }

    /** Jumlah frame audio yang sudah diproses sejak stream dibuka. */
    int64_t framesProcessed() const { return framesProcessed_.load(); }

    /** Jumlah kejadian underrun/overrun (xrun) yang dilaporkan Oboe. */
    int64_t xrunCount() const;

    /** Akses ke rangkaian efek untuk pengaturan dari UI. */
    EffectChain& effects() { return effects_; }

    /** Akses ke detektor pitch untuk tuner (BAB 14). */
    PitchDetector& pitch() { return pitch_; }

    /** Pemantau performa callback (BAB 16). */
    PerformanceMonitor& performance() { return performance_; }

    /**
     * Estimasi latensi round-trip (BAB 16) dalam milidetik, dihitung dari
     * ukuran buffer input+output pada sample rate stream. Ini estimasi berbasis
     * buffer nyata perangkat, bukan angka teoretis.
     */
    float estimatedLatencyMs() const;

    /**
     * Latensi round-trip TERUKUR (BAB 16) dalam milidetik, dihitung dari
     * timestamp perangkat yang dilaporkan Oboe (posisi frame yang sudah
     * ditulis/dibaca versus yang sudah diputar/ditangkap). Mengembalikan nilai
     * negatif bila perangkat tidak menyediakan timestamp yang dapat dipercaya.
     */
    float measuredLatencyMs() const;

    /** Kapasitas buffer stream (frame), 0 bila belum dibuka. */
    int32_t bufferCapacityFrames() const;

    /** Trim gain input (BAB 15), dalam desibel. */
    GainStage& inputGain() { return inputGain_; }

    /** Level output master (BAB 15), dalam desibel. */
    GainStage& outputGain() { return outputGain_; }

    /** true bila input terdeteksi terpotong (clipping) pada blok terakhir. */
    bool inputClipping() const { return inputClipping_.load(); }

    /** true bila output terdeteksi terpotong pada blok terakhir. */
    bool outputClipping() const { return outputClipping_.load(); }

    /**
     * Ducking (BAB 15): menurunkan level keluaran sementara saat audio focus
     * sementara hilang tetapi aplikasi masih boleh berbunyi. Perubahan
     * dihaluskan lewat [GainStage] agar tidak menimbulkan klik.
     */
    void setDucked(bool ducked) {
        ducked_.store(ducked);
        duckGain_.setTargetDb(ducked ? kDuckDb : 0.0f);
    }

    bool ducked() const { return ducked_.load(); }

    /** Batas atas level keluaran (limiter) dalam desibel, default 0 dBFS. */
    void setLimiterCeilingDb(float db) { limiterCeilingDb_.store(db); }

    float limiterCeilingDb() const { return limiterCeilingDb_.load(); }

    /**
     * Pesan kesalahan terakhir (BAB 15), mis. perangkat terputus. Kosong bila
     * tidak ada masalah. Dibaca UI untuk menampilkan status yang informatif.
     */
    std::string lastError() const {
        std::lock_guard<std::mutex> guard(errorLock_);
        return lastError_;
    }

    /** Mengosongkan pesan kesalahan terakhir. */
    void clearLastError() {
        std::lock_guard<std::mutex> guard(errorLock_);
        lastError_.clear();
    }

    /**
     * Memilih perangkat input/output (BAB 5). ID berasal dari
     * AudioDeviceInfo.id pada sisi Android. Nilai -1 berarti perangkat default
     * sistem. Perubahan berlaku pada pembukaan stream berikutnya.
     */
    void setDeviceIds(int32_t inputDeviceId, int32_t outputDeviceId) {
        inputDeviceId_.store(inputDeviceId);
        outputDeviceId_.store(outputDeviceId);
    }

    /**
     * Menandai apakah perangkat input yang dipilih terhubung ke jack headset
     * (mis. iRig yang dicolok ke lubang headset). Dipakai untuk memilih
     * InputPreset::VoiceRecognition, yang pada banyak perangkat membuka jalur
     * mikrofon headset (TRRS) dengan benar.
     */
    void setHeadsetInput(bool headset) { headsetInput_.store(headset); }

    /**
     * Nama paket aplikasi. Wajib diisi untuk stream input pada Android 12+
     * (API 31+): tanpa ini stream input dapat gagal karena pemeriksaan izin.
     */
    void setPackageName(std::string packageName) {
        std::lock_guard<std::mutex> guard(errorLock_);
        packageName_ = std::move(packageName);
    }

    /**
     * Sample rate yang diinginkan (BAB 15). Oboe akan memakai nilai ini bila
     * didukung perangkat; bila tidak, stream dibuka pada rate bawaan perangkat
     * dan [sampleRate] melaporkan rate sebenarnya. Berlaku pada pembukaan
     * stream berikutnya.
     */
    void setPreferredSampleRate(int32_t sampleRate) {
        if (sampleRate > 0) {
            preferredSampleRate_.store(sampleRate);
        }
    }

    int32_t preferredSampleRate() const { return preferredSampleRate_.load(); }

    std::string version() const;

    // oboe::FullDuplexStream
    oboe::DataCallbackResult onBothStreamsReady(const void *inputData, int numInputFrames,
                                                void *outputData, int numOutputFrames) override;

    // oboe::AudioStreamErrorCallback
    void onErrorAfterClose(oboe::AudioStream *stream, oboe::Result error) override;

private:
    oboe::Result openStreams();
    oboe::Result openOutputStream(oboe::AudioStreamBuilder& builder);
    oboe::Result openInputStream(oboe::AudioStreamBuilder& builder);

    std::shared_ptr<oboe::AudioStream> inputStream_;
    std::shared_ptr<oboe::AudioStream> outputStream_;
    std::atomic<bool> running_{false};
    std::atomic<int32_t> sampleRate_{0};
    std::atomic<int32_t> framesPerBurst_{0};
    std::atomic<int32_t> channelCount_{0};
    std::atomic<int32_t> inputDeviceId_{-1};
    std::atomic<int32_t> outputDeviceId_{-1};
    std::atomic<bool> headsetInput_{false};
    std::atomic<int32_t> preferredSampleRate_{48000};
    std::atomic<float> inputLevel_{0.0f};
    std::atomic<float> outputLevel_{0.0f};
    std::atomic<int64_t> callbackCount_{0};
    std::atomic<int64_t> framesProcessed_{0};
    EffectChain effects_;
    PitchDetector pitch_;
    PerformanceMonitor performance_;
    GainStage inputGain_;
    GainStage outputGain_;
    std::atomic<bool> inputClipping_{false};
    std::atomic<bool> outputClipping_{false};
    std::atomic<float> limiterCeilingDb_{0.0f};
    std::atomic<bool> ducked_{false};
    GainStage duckGain_;
    static constexpr float kDuckDb = -12.0f;
    std::mutex lock_;  // Melindungi pembukaan/penutupan stream, bukan callback.
    mutable std::mutex errorLock_;
    std::string lastError_;
    std::string packageName_;
};

}  // namespace alto

#endif  // ALTOGUITARFX_AUDIOENGINE_H
