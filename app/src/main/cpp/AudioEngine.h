#ifndef ALTOGUITARFX_AUDIOENGINE_H
#define ALTOGUITARFX_AUDIOENGINE_H

#include <oboe/FullDuplexStream.h>
#include <oboe/Oboe.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

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

    /**
     * Memilih perangkat input/output (BAB 5). ID berasal dari
     * AudioDeviceInfo.id pada sisi Android. Nilai -1 berarti perangkat default
     * sistem. Perubahan berlaku pada pembukaan stream berikutnya.
     */
    void setDeviceIds(int32_t inputDeviceId, int32_t outputDeviceId) {
        inputDeviceId_.store(inputDeviceId);
        outputDeviceId_.store(outputDeviceId);
    }

    std::string version() const;

    // oboe::FullDuplexStream
    oboe::DataCallbackResult onBothStreamsReady(const void *inputData, int numInputFrames,
                                                void *outputData, int numOutputFrames) override;

    // oboe::AudioStreamErrorCallback
    void onErrorAfterClose(oboe::AudioStream *stream, oboe::Result error) override;

private:
    oboe::Result openStreams();

    std::shared_ptr<oboe::AudioStream> inputStream_;
    std::shared_ptr<oboe::AudioStream> outputStream_;
    std::atomic<bool> running_{false};
    std::atomic<int32_t> sampleRate_{0};
    std::atomic<int32_t> framesPerBurst_{0};
    std::atomic<int32_t> channelCount_{0};
    std::atomic<int32_t> inputDeviceId_{-1};
    std::atomic<int32_t> outputDeviceId_{-1};
    std::atomic<float> inputLevel_{0.0f};
    std::atomic<float> outputLevel_{0.0f};
    std::atomic<int64_t> callbackCount_{0};
    std::atomic<int64_t> framesProcessed_{0};
    std::mutex lock_;  // Melindungi pembukaan/penutupan stream, bukan callback.
};

}  // namespace alto

#endif  // ALTOGUITARFX_AUDIOENGINE_H
