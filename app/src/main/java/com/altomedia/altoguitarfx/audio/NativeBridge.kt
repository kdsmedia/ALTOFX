package com.altomedia.altoguitarfx.audio

/**
 * Jembatan ke mesin audio native (BAB 3).
 *
 * Library native di-load sekali saat kelas pertama dipakai. Semua fungsi
 * native bersifat statis agar tidak perlu mengelola objek dari sisi Kotlin.
 */
object NativeBridge {

    /** true bila library native berhasil dimuat. */
    val isLoaded: Boolean = try {
        System.loadLibrary("altoguitarfx")
        true
    } catch (t: UnsatisfiedLinkError) {
        false
    }

    /** Versi mesin native, atau pesan kesalahan bila library gagal dimuat. */
    fun version(): String =
        if (isLoaded) nativeVersion() else "Native library tidak tersedia"

    /** Mengembalikan input apa adanya; dipakai untuk memverifikasi jalur JNI. */
    fun echo(input: String): String =
        if (isLoaded) nativeEcho(input) else input

    // ---- Kontrol audio (BAB 4) ----

    fun start(): String = if (isLoaded) nativeStart() else "Native library tidak tersedia"

    fun stop(): String = if (isLoaded) nativeStop() else "Native library tidak tersedia"

    fun isRunning(): Boolean = isLoaded && nativeIsRunning()

    /** Sample rate stream aktif, 0 bila belum berjalan. */
    fun sampleRate(): Int = if (isLoaded) nativeSampleRate() else 0

    /** Jumlah frame per callback, 0 bila belum berjalan. */
    fun framesPerBurst(): Int = if (isLoaded) nativeFramesPerBurst() else 0

    /**
     * Memilih perangkat input/output (BAB 5). ID berasal dari
     * AudioDeviceInfo.id; gunakan -1 untuk perangkat default sistem.
     */
    fun setDeviceIds(inputDeviceId: Int, outputDeviceId: Int) {
        if (isLoaded) nativeSetDeviceIds(inputDeviceId, outputDeviceId)
    }

    // ---- Level & diagnostik (BAB 6) ----

    /** Level puncak blok input terakhir, 0..1 linier. */
    fun inputLevel(): Float = if (isLoaded) nativeInputLevel() else 0f

    /** Level puncak blok output terakhir, 0..1 linier. */
    fun outputLevel(): Float = if (isLoaded) nativeOutputLevel() else 0f

    fun callbackCount(): Long = if (isLoaded) nativeCallbackCount() else 0L

    fun framesProcessed(): Long = if (isLoaded) nativeFramesProcessed() else 0L

    fun xrunCount(): Long = if (isLoaded) nativeXrunCount() else 0L

    // ---- Kontrol efek (BAB 7+) ----

    fun setEffectEnabled(type: Int, enabled: Boolean) {
        if (isLoaded) nativeSetEffectEnabled(type, enabled)
    }

    fun isEffectEnabled(type: Int): Boolean =
        if (isLoaded) nativeIsEffectEnabled(type) else false

    fun setEffectParam(type: Int, index: Int, value: Float) {
        if (isLoaded) nativeSetEffectParam(type, index, value)
    }

    fun effectParam(type: Int, index: Int): Float =
        if (isLoaded) nativeGetEffectParam(type, index) else 0f

    fun effectParamCount(type: Int): Int =
        if (isLoaded) nativeEffectParamCount(type) else 0

    fun effectName(type: Int): String =
        if (isLoaded) nativeEffectName(type) else ""

    // ---- Pengelola urutan efek (BAB 12) ----

    /** Jumlah efek dalam rangkaian. */
    fun effectCount(): Int = if (isLoaded) nativeEffectCount() else 0

    /** ID efek pada slot urutan tertentu, -1 bila slot tidak valid. */
    fun effectOrderAt(slot: Int): Int = if (isLoaded) nativeEffectOrderAt(slot) else -1

    /** Menggeser efek pada urutan: delta negatif naik, positif turun. */
    fun moveEffect(type: Int, delta: Int) {
        if (isLoaded) nativeMoveEffect(type, delta)
    }

    /** Mengembalikan urutan efek ke bawaan. */
    fun resetEffectOrder() {
        if (isLoaded) nativeResetEffectOrder()
    }

    /**
     * Menetapkan urutan efek sekaligus. Larik harus memuat permutasi lengkap
     * semua ID efek; nilai tidak valid diabaikan di sisi native.
     */
    fun setEffectOrder(order: IntArray) {
        if (isLoaded) nativeSetEffectOrder(order)
    }

    // ---- Tuner / deteksi pitch (BAB 14) ----

    /** Mengaktifkan analisis pitch; hanya saat layar tuner dibuka. */
    fun setTunerActive(active: Boolean) {
        if (isLoaded) nativeSetTunerActive(active)
    }

    /** Frekuensi dasar terakhir (Hz), 0 bila tidak ada nada. */
    fun tunerFrequency(): Float = if (isLoaded) nativeTunerFrequency() else 0f

    /** Nada MIDI terakhir (69 = A4), -1 bila tidak ada nada. */
    fun tunerMidiNote(): Float = if (isLoaded) nativeTunerMidiNote() else -1f

    /** Selisih dari nada terdekat dalam cents. */
    fun tunerCents(): Float = if (isLoaded) nativeTunerCents() else 0f

    /** Keyakinan deteksi 0..1. */
    fun tunerConfidence(): Float = if (isLoaded) nativeTunerConfidence() else 0f

    /** Mengatur referensi A4 dalam Hz (default 440). */
    fun setTunerReference(referenceHz: Float) {
        if (isLoaded) nativeSetTunerReference(referenceHz)
    }

    // ---- Pengaturan audio, gain, limiter, error (BAB 15) ----

    /** Trim gain input dalam desibel. */
    fun setInputGainDb(db: Float) {
        if (isLoaded) nativeSetInputGainDb(db)
    }

    fun inputGainDb(): Float = if (isLoaded) nativeInputGainDb() else 0f

    /** Level output master dalam desibel. */
    fun setOutputGainDb(db: Float) {
        if (isLoaded) nativeSetOutputGainDb(db)
    }

    fun outputGainDb(): Float = if (isLoaded) nativeOutputGainDb() else 0f

    /** Batas limiter keluaran dalam desibel (0 = 0 dBFS). */
    fun setLimiterCeilingDb(db: Float) {
        if (isLoaded) nativeSetLimiterCeilingDb(db)
    }

    fun limiterCeilingDb(): Float = if (isLoaded) nativeLimiterCeilingDb() else 0f

    /** true bila input terpotong pada blok terakhir. */
    fun inputClipping(): Boolean = isLoaded && nativeInputClipping()

    /** true bila output terpotong pada blok terakhir. */
    fun outputClipping(): Boolean = isLoaded && nativeOutputClipping()

    /** Pesan kesalahan audio terakhir, kosong bila tidak ada. */
    fun lastError(): String = if (isLoaded) nativeLastError() else ""

    /** Mengosongkan pesan kesalahan terakhir. */
    fun clearLastError() {
        if (isLoaded) nativeClearLastError()
    }

    private external fun nativeVersion(): String

    private external fun nativeEcho(input: String): String

    private external fun nativeStart(): String

    private external fun nativeStop(): String

    private external fun nativeIsRunning(): Boolean

    private external fun nativeSampleRate(): Int

    private external fun nativeFramesPerBurst(): Int

    private external fun nativeSetDeviceIds(inputDeviceId: Int, outputDeviceId: Int)

    private external fun nativeInputLevel(): Float

    private external fun nativeOutputLevel(): Float

    private external fun nativeCallbackCount(): Long

    private external fun nativeFramesProcessed(): Long

    private external fun nativeXrunCount(): Long

    private external fun nativeSetEffectEnabled(type: Int, enabled: Boolean)

    private external fun nativeIsEffectEnabled(type: Int): Boolean

    private external fun nativeSetEffectParam(type: Int, index: Int, value: Float)

    private external fun nativeGetEffectParam(type: Int, index: Int): Float

    private external fun nativeEffectParamCount(type: Int): Int

    private external fun nativeEffectName(type: Int): String

    private external fun nativeEffectCount(): Int

    private external fun nativeEffectOrderAt(slot: Int): Int

    private external fun nativeMoveEffect(type: Int, delta: Int)

    private external fun nativeResetEffectOrder()

    private external fun nativeSetEffectOrder(order: IntArray)

    private external fun nativeSetTunerActive(active: Boolean)

    private external fun nativeTunerFrequency(): Float

    private external fun nativeTunerMidiNote(): Float

    private external fun nativeTunerCents(): Float

    private external fun nativeTunerConfidence(): Float

    private external fun nativeSetTunerReference(referenceHz: Float)

    private external fun nativeSetInputGainDb(db: Float)

    private external fun nativeInputGainDb(): Float

    private external fun nativeSetOutputGainDb(db: Float)

    private external fun nativeOutputGainDb(): Float

    private external fun nativeSetLimiterCeilingDb(db: Float)

    private external fun nativeLimiterCeilingDb(): Float

    private external fun nativeInputClipping(): Boolean

    private external fun nativeOutputClipping(): Boolean

    private external fun nativeLastError(): String

    private external fun nativeClearLastError()
}
