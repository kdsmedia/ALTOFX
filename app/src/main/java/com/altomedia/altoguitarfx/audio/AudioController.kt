package com.altomedia.altoguitarfx.audio

/**
 * Kontrol audio tingkat aplikasi (BAB 4-5).
 *
 * Menyimpan pilihan perangkat input/output lalu meneruskannya ke engine
 * native. UI cukup memanggil [selectInput]/[selectOutput], [start], [stop],
 * dan membaca [Status] tanpa mengetahui detail JNI.
 */
object AudioController {

    const val DEFAULT_DEVICE_ID = -1

    data class Status(
        val running: Boolean,
        val message: String,
        val sampleRate: Int,
        val framesPerBurst: Int
    )

    private var inputDeviceId: Int = DEFAULT_DEVICE_ID
    private var outputDeviceId: Int = DEFAULT_DEVICE_ID

    fun selectInput(deviceId: Int) {
        inputDeviceId = deviceId
    }

    fun selectOutput(deviceId: Int) {
        outputDeviceId = deviceId
    }

    /**
     * Mengganti input lalu menerapkannya segera. Bila audio sedang berjalan,
     * stream dihentikan dan dimulai ulang agar perangkat baru benar-benar
     * dipakai (blueprint bagian 6: pilihan input di Home).
     */
    fun switchInput(deviceId: Int): Status {
        val wasRunning = NativeBridge.isRunning()
        if (wasRunning) {
            NativeBridge.stop()
        }
        selectInput(deviceId)
        return if (wasRunning) start() else refresh()
    }

    /** Mengganti output lalu menerapkannya segera (lihat [switchInput]). */
    fun switchOutput(deviceId: Int): Status {
        val wasRunning = NativeBridge.isRunning()
        if (wasRunning) {
            NativeBridge.stop()
        }
        selectOutput(deviceId)
        return if (wasRunning) start() else refresh()
    }

    /**
     * Dipanggil saat perangkat input aktif dicabut (blueprint BAB 10 & 15:
     * 'perangkat dilepas'). Bila audio berjalan, stream dihentikan dan
     * dimulai ulang dengan perangkat bawaan agar suara tidak hilang diam-diam.
     * Mengembalikan Status baru, atau null bila tidak ada yang perlu dilakukan.
     */
    fun handleInputDisconnected(deviceId: Int): Status? {
        if (deviceId != inputDeviceId) return null
        inputDeviceId = DEFAULT_DEVICE_ID
        return if (NativeBridge.isRunning()) start() else refresh()
    }

    /** Lihat [handleInputDisconnected] untuk perangkat output. */
    fun handleOutputDisconnected(deviceId: Int): Status? {
        if (deviceId != outputDeviceId) return null
        outputDeviceId = DEFAULT_DEVICE_ID
        return if (NativeBridge.isRunning()) start() else refresh()
    }

    /**
     * Ducking: menurunkan level keluaran sementara saat audio focus sementara
     * hilang tetapi aplikasi masih boleh berbunyi (BAB 15).
     */
    fun setDucked(ducked: Boolean) {
        NativeBridge.setDucked(ducked)
    }

    fun selectedInputId(): Int = inputDeviceId

    fun selectedOutputId(): Int = outputDeviceId

    fun start(): Status {
        NativeBridge.setDeviceIds(inputDeviceId, outputDeviceId)
        val message = NativeBridge.start()
        return current(message)
    }

    fun stop(): Status {
        val message = NativeBridge.stop()
        return current(message)
    }

    fun refresh(): Status = current("")

    // ---- Lifecycle & interupsi (BAB 15) ----

    private var interrupted = false

    /**
     * Menjeda audio karena aplikasi masuk background atau kehilangan audio
     * focus. Status berjalan diingat agar bisa dilanjutkan lewat [resume].
     */
    fun pauseForInterruption() {
        if (NativeBridge.isRunning()) {
            NativeBridge.stop()
            interrupted = true
        }
    }

    /** Melanjutkan audio setelah [pauseForInterruption]. */
    fun resumeAfterInterruption() {
        if (interrupted) {
            NativeBridge.setDeviceIds(inputDeviceId, outputDeviceId)
            NativeBridge.start()
            interrupted = false
        }
    }

    /** true bila audio sedang dijeda karena interupsi. */
    fun isInterrupted(): Boolean = interrupted

    // ---- Gain, limiter, dan pemantauan clipping (BAB 15) ----

    data class Levels(
        val inputClipping: Boolean,
        val outputClipping: Boolean,
        val lastError: String
    )

    fun setInputGainDb(db: Float) = NativeBridge.setInputGainDb(db)

    fun inputGainDb(): Float = NativeBridge.inputGainDb()

    fun setOutputGainDb(db: Float) = NativeBridge.setOutputGainDb(db)

    fun outputGainDb(): Float = NativeBridge.outputGainDb()

    fun setLimiterCeilingDb(db: Float) = NativeBridge.setLimiterCeilingDb(db)

    fun limiterCeilingDb(): Float = NativeBridge.limiterCeilingDb()

    fun levels(): Levels = Levels(
        inputClipping = NativeBridge.inputClipping(),
        outputClipping = NativeBridge.outputClipping(),
        lastError = NativeBridge.lastError()
    )

    // ---- Performa & latensi (BAB 16) ----

    data class Performance(
        val averageCallbackMicros: Double,
        val peakCallbackMicros: Double,
        val averageCpuLoad: Float,
        val peakCpuLoad: Float,
        val estimatedLatencyMs: Float,
        val bufferCapacityFrames: Int
    )

    fun performance(): Performance = Performance(
        averageCallbackMicros = NativeBridge.averageCallbackMicros(),
        peakCallbackMicros = NativeBridge.peakCallbackMicros(),
        averageCpuLoad = NativeBridge.averageCpuLoad(),
        peakCpuLoad = NativeBridge.peakCpuLoad(),
        estimatedLatencyMs = NativeBridge.estimatedLatencyMs(),
        bufferCapacityFrames = NativeBridge.bufferCapacityFrames()
    )

    fun resetPerformance() = NativeBridge.resetPerformance()

    // ---- Mute & bypass keseluruhan (prinsip interaksi blueprint) ----

    fun setMuted(muted: Boolean) = NativeBridge.setMuted(muted)

    fun isMuted(): Boolean = NativeBridge.isMuted()

    fun setGlobalBypass(bypass: Boolean) = NativeBridge.setGlobalBypass(bypass)

    fun isGlobalBypass(): Boolean = NativeBridge.isGlobalBypass()

    /** Data level & diagnostik untuk level meter (BAB 6). */
    data class MeterData(
        val inputLevel: Float,
        val outputLevel: Float,
        val callbackCount: Long,
        val framesProcessed: Long,
        val xrunCount: Long
    )

    fun meter(): MeterData = MeterData(
        inputLevel = NativeBridge.inputLevel(),
        outputLevel = NativeBridge.outputLevel(),
        callbackCount = NativeBridge.callbackCount(),
        framesProcessed = NativeBridge.framesProcessed(),
        xrunCount = NativeBridge.xrunCount()
    )

    private fun current(message: String): Status = Status(
        running = NativeBridge.isRunning(),
        message = message,
        sampleRate = NativeBridge.sampleRate(),
        framesPerBurst = NativeBridge.framesPerBurst()
    )
}
