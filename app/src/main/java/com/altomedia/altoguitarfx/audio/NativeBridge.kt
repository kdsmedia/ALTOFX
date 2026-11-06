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

    private external fun nativeVersion(): String

    private external fun nativeEcho(input: String): String
}
