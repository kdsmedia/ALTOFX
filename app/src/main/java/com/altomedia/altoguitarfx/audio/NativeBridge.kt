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

    private external fun nativeVersion(): String

    private external fun nativeEcho(input: String): String

    private external fun nativeStart(): String

    private external fun nativeStop(): String

    private external fun nativeIsRunning(): Boolean

    private external fun nativeSampleRate(): Int

    private external fun nativeFramesPerBurst(): Int

    private external fun nativeSetDeviceIds(inputDeviceId: Int, outputDeviceId: Int)
}
