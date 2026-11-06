package com.altomedia.altoguitarfx.audio

/**
 * Kontrol audio tingkat aplikasi (BAB 4).
 *
 * Membungkus [NativeBridge] agar UI cukup memanggil start/stop dan membaca
 * status tanpa mengetahui detail JNI.
 */
object AudioController {

    data class Status(
        val running: Boolean,
        val message: String,
        val sampleRate: Int,
        val framesPerBurst: Int
    )

    fun start(): Status {
        val message = NativeBridge.start()
        return current(message)
    }

    fun stop(): Status {
        val message = NativeBridge.stop()
        return current(message)
    }

    fun refresh(): Status = current("")

    private fun current(message: String): Status = Status(
        running = NativeBridge.isRunning(),
        message = message,
        sampleRate = NativeBridge.sampleRate(),
        framesPerBurst = NativeBridge.framesPerBurst()
    )
}
