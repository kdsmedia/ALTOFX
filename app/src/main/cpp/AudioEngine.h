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
    std::mutex lock_;  // Melindungi pembukaan/penutupan stream, bukan callback.
};

}  // namespace alto

#endif  // ALTOGUITARFX_AUDIOENGINE_H
