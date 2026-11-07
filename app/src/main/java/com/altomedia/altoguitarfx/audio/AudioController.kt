package com.altomedia.altoguitarfx.audio

import android.content.Context
import android.content.SharedPreferences
import android.media.AudioDeviceInfo
import android.media.AudioManager

/**
 * Kontrol audio tingkat aplikasi (BAB 4-5).
 *
 * Menyimpan pilihan perangkat input/output lalu meneruskannya ke engine
 * native. UI cukup memanggil [selectInput]/[selectOutput], [start], [stop],
 * dan membaca [Status] tanpa mengetahui detail JNI.
 */
object AudioController {

    const val DEFAULT_DEVICE_ID = -1

    /** Sample rate default sesuai blueprint Lampiran A. */
    const val DEFAULT_SAMPLE_RATE = 48000

    /** Referensi tuning standar (A4 = 440 Hz, BAB 14). */
    const val DEFAULT_TUNER_REFERENCE = 440f

    data class Status(
        val running: Boolean,
        val message: String,
        val sampleRate: Int,
        val framesPerBurst: Int
    )

    private var inputDeviceId: Int = DEFAULT_DEVICE_ID
    private var outputDeviceId: Int = DEFAULT_DEVICE_ID
    private var preferredSampleRate: Int = DEFAULT_SAMPLE_RATE
    private var prefs: SharedPreferences? = null
    private var appContext: Context? = null

    /**
     * Memuat pilihan perangkat dan sample rate yang tersimpan (blueprint
     * BAB 5/15). Dipanggil sekali dari MainActivity sebelum audio dipakai,
     * sehingga pilihan pengguna bertahan setelah aplikasi ditutup.
     */
    fun init(context: Context) {
        val store = context.applicationContext
            .getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        prefs = store
        appContext = context.applicationContext
        inputDeviceId = store.getInt(KEY_INPUT_DEVICE, DEFAULT_DEVICE_ID)
        outputDeviceId = store.getInt(KEY_OUTPUT_DEVICE, DEFAULT_DEVICE_ID)
        preferredSampleRate =
            store.getInt(KEY_SAMPLE_RATE, DEFAULT_SAMPLE_RATE).takeIf { it > 0 }
                ?: DEFAULT_SAMPLE_RATE
        // Nama paket wajib untuk stream input pada Android 12+ (API 31+).
        NativeBridge.setPackageName(context.packageName)
        NativeBridge.setDeviceIds(inputDeviceId, outputDeviceId)
        NativeBridge.setPreferredSampleRate(preferredSampleRate)
        NativeBridge.setTunerReference(
            store.getFloat(KEY_TUNER_REFERENCE, DEFAULT_TUNER_REFERENCE)
        )
        NativeBridge.setInputGainDb(store.getFloat(KEY_INPUT_GAIN_DB, 0f))
        NativeBridge.setOutputGainDb(store.getFloat(KEY_OUTPUT_GAIN_DB, 0f))
        NativeBridge.setMuted(store.getBoolean(KEY_MUTED, false))
        NativeBridge.setLimiterCeilingDb(store.getFloat(KEY_LIMITER_DB, 0f))
        NativeBridge.setGlobalBypass(store.getBoolean(KEY_GLOBAL_BYPASS, false))
    }

    /**
     * Referensi tuning A4 yang tersimpan (BAB 14). Bertahan antar sesi
     * seperti pilihan perangkat.
     */
    fun tunerReference(): Float =
        prefs?.getFloat(KEY_TUNER_REFERENCE, DEFAULT_TUNER_REFERENCE)
            ?: DEFAULT_TUNER_REFERENCE

    /** Menyimpan referensi tuning A4 (BAB 14). */
    fun setTunerReference(referenceHz: Float) {
        prefs?.edit()?.putFloat(KEY_TUNER_REFERENCE, referenceHz)?.apply()
        NativeBridge.setTunerReference(referenceHz)
    }

    fun selectInput(deviceId: Int) {
        inputDeviceId = deviceId
        prefs?.edit()?.putInt(KEY_INPUT_DEVICE, deviceId)?.apply()
        applyHeadsetInputFlag()
    }

    fun selectOutput(deviceId: Int) {
        outputDeviceId = deviceId
        prefs?.edit()?.putInt(KEY_OUTPUT_DEVICE, deviceId)?.apply()
    }

    /**
     * Memberi tahu engine native apakah input terpilih berupa perangkat yang
     * terhubung ke jack headset (headset kabel / USB-C-to-3.5mm). Ini penting
     * untuk iRig yang dicolok ke lubang headset: pada banyak perangkat, preset
     * input VoiceRecognition-lah yang membuka mikrofon TRRS dengan benar.
     *
     * Bila perangkat tidak dapat dipetakan (mis. perangkat sudah dicabut),
     * tidak ada yang diubah; jalur native tetap punya fallback sendiri.
     */
    private fun applyHeadsetInputFlag() {
        val context = appContext ?: return
        val id = inputDeviceId
        if (id == DEFAULT_DEVICE_ID) {
            NativeBridge.setHeadsetInput(false)
            return
        }
        val device = runCatching {
            val manager = context.getSystemService(Context.AUDIO_SERVICE) as AudioManager
            manager.getDevices(AudioManager.GET_DEVICES_INPUTS).firstOrNull { it.id == id }
        }.getOrNull() ?: return
        val headset = when (device.type) {
            AudioDeviceInfo.TYPE_WIRED_HEADSET,
            AudioDeviceInfo.TYPE_WIRED_HEADPHONES,
            AudioDeviceInfo.TYPE_USB_HEADSET,
            AudioDeviceInfo.TYPE_USB_DEVICE,
            AudioDeviceInfo.TYPE_USB_ACCESSORY -> true
            else -> false
        }
        NativeBridge.setHeadsetInput(headset)
    }

    /**
     * Memilih sample rate (BAB 15). Bila audio sedang berjalan, stream
     * dihentikan dan dimulai ulang agar rate baru benar-benar dipakai.
     */
    fun setPreferredSampleRate(sampleRate: Int): Status {
        preferredSampleRate = sampleRate
        prefs?.edit()?.putInt(KEY_SAMPLE_RATE, sampleRate)?.apply()
        NativeBridge.setPreferredSampleRate(sampleRate)
        return if (NativeBridge.isRunning()) start() else refresh()
    }

    fun selectedSampleRate(): Int = preferredSampleRate

    /** Sample rate stream yang sedang berjalan, 0 bila belum berjalan. */
    fun sampleRate(): Int = NativeBridge.sampleRate()

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
        // Simpan reset agar sesi berikutnya tidak mencoba perangkat yang hilang.
        prefs?.edit()?.putInt(KEY_INPUT_DEVICE, DEFAULT_DEVICE_ID)?.apply()
        return if (NativeBridge.isRunning()) start() else refresh()
    }

    /** Lihat [handleInputDisconnected] untuk perangkat output. */
    fun handleOutputDisconnected(deviceId: Int): Status? {
        if (deviceId != outputDeviceId) return null
        outputDeviceId = DEFAULT_DEVICE_ID
        prefs?.edit()?.putInt(KEY_OUTPUT_DEVICE, DEFAULT_DEVICE_ID)?.apply()
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
        applyHeadsetInputFlag()
        NativeBridge.setDeviceIds(inputDeviceId, outputDeviceId)
        NativeBridge.setPreferredSampleRate(preferredSampleRate)
        val message = NativeBridge.start()
        // Hanya ingat niat bila stream benar-benar berjalan; bila gagal
        // (mis. izin ditolak), jangan mencoba memulihkan otomatis.
        userWantsAudio = NativeBridge.isRunning()
        return current(message)
    }

    fun stop(): Status {
        userWantsAudio = false
        val message = NativeBridge.stop()
        return current(message)
    }

    fun refresh(): Status = current("")

    // ---- Lifecycle & interupsi (BAB 15) ----

    private var interrupted = false

    /**
     * true bila pengguna memang ingin audio berbunyi (menekan Start). Dipakai
     * agar pemulihan otomatis (onStart / audio focus) tidak menyalakan audio
     * yang sengaja dimatikan pengguna.
     */
    private var userWantsAudio = false

    /**
     * Menjeda audio karena aplikasi masuk background atau kehilangan audio
     * focus. Keinginan pengguna diingat agar bisa dilanjutkan lewat
     * [resumeAfterInterruption].
     */
    fun pauseForInterruption() {
        if (NativeBridge.isRunning()) {
            NativeBridge.stop()
            // Bersihkan ekor delay/reverb (blueprint bagian 7) agar tidak
            // berbunyi lagi setelah stream dimulai ulang.
            NativeBridge.resetEffects()
            interrupted = true
        }
    }

    /**
     * Melanjutkan audio setelah [pauseForInterruption]. Hanya berjalan bila
     * pengguna sebelumnya menekan Start; tidak menyalakan audio yang sudah
     * dihentikan lewat tombol Stop.
     */
    fun resumeAfterInterruption() {
        // Selalu bersihkan flag agar pemulihan berikutnya tidak terblokir.
        interrupted = false
        if (!userWantsAudio || NativeBridge.isRunning()) return
        NativeBridge.setDeviceIds(inputDeviceId, outputDeviceId)
        NativeBridge.setPreferredSampleRate(preferredSampleRate)
        NativeBridge.start()
    }

    /** true bila audio sedang dijeda karena interupsi. */
    fun isInterrupted(): Boolean = interrupted

    // ---- Gain, limiter, dan pemantauan clipping (BAB 15) ----

    data class Levels(
        val inputClipping: Boolean,
        val outputClipping: Boolean,
        val lastError: String
    )

    fun setInputGainDb(db: Float) {
        prefs?.edit()?.putFloat(KEY_INPUT_GAIN_DB, db)?.apply()
        NativeBridge.setInputGainDb(db)
    }

    fun inputGainDb(): Float = NativeBridge.inputGainDb()

    fun setOutputGainDb(db: Float) {
        prefs?.edit()?.putFloat(KEY_OUTPUT_GAIN_DB, db)?.apply()
        NativeBridge.setOutputGainDb(db)
    }

    fun outputGainDb(): Float = NativeBridge.outputGainDb()

    fun setLimiterCeilingDb(db: Float) {
        prefs?.edit()?.putFloat(KEY_LIMITER_DB, db)?.apply()
        NativeBridge.setLimiterCeilingDb(db)
    }

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
        val measuredLatencyMs: Float,
        val bufferCapacityFrames: Int,
        val xrunCount: Long
    )

    fun performance(): Performance = Performance(
        averageCallbackMicros = NativeBridge.averageCallbackMicros(),
        peakCallbackMicros = NativeBridge.peakCallbackMicros(),
        averageCpuLoad = NativeBridge.averageCpuLoad(),
        peakCpuLoad = NativeBridge.peakCpuLoad(),
        estimatedLatencyMs = NativeBridge.estimatedLatencyMs(),
        measuredLatencyMs = NativeBridge.measuredLatencyMs(),
        bufferCapacityFrames = NativeBridge.bufferCapacityFrames(),
        xrunCount = NativeBridge.xrunCount()
    )

    fun resetPerformance() = NativeBridge.resetPerformance()

    // ---- Mute & bypass keseluruhan (prinsip interaksi blueprint) ----

    fun setMuted(muted: Boolean) {
        prefs?.edit()?.putBoolean(KEY_MUTED, muted)?.apply()
        NativeBridge.setMuted(muted)
    }

    fun isMuted(): Boolean = NativeBridge.isMuted()

    fun setGlobalBypass(bypass: Boolean) {
        prefs?.edit()?.putBoolean(KEY_GLOBAL_BYPASS, bypass)?.apply()
        NativeBridge.setGlobalBypass(bypass)
    }

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

    private const val PREFS_NAME = "alto_audio"
    private const val KEY_INPUT_DEVICE = "input_device"
    private const val KEY_OUTPUT_DEVICE = "output_device"
    private const val KEY_SAMPLE_RATE = "sample_rate"
    private const val KEY_TUNER_REFERENCE = "tuner_reference"
    private const val KEY_INPUT_GAIN_DB = "input_gain_db"
    private const val KEY_OUTPUT_GAIN_DB = "output_gain_db"
    private const val KEY_MUTED = "muted"
    private const val KEY_LIMITER_DB = "limiter_db"
    private const val KEY_GLOBAL_BYPASS = "global_bypass"

    private fun current(message: String): Status = Status(
        running = NativeBridge.isRunning(),
        message = message,
        sampleRate = NativeBridge.sampleRate(),
        framesPerBurst = NativeBridge.framesPerBurst()
    )
}
