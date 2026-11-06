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
}
