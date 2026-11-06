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
