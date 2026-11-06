package com.altomedia.altoguitarfx.data

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue

/**
 * Preset yang sedang aktif (blueprint bagian 6: Home menampilkan preset aktif).
 *
 * Menyimpan nama preset terakhir yang dimuat/disimpan agar Home dan layar
 * Presets menampilkan keadaan yang sama. Nilai berupa Compose state sehingga
 * UI otomatis ikut berubah.
 */
object ActivePresetStore {

    /** Nama preset aktif, null bila belum ada. */
    var name by mutableStateOf<String?>(null)
        private set

    /** true bila preset aktif adalah preset pengguna (bukan bawaan). */
    var isUserPreset by mutableStateOf(false)
        private set

    /** Menandai preset yang baru dimuat/disimpan. */
    fun set(preset: Preset) {
        name = preset.name
        isUserPreset = !preset.builtIn
    }

    /** Menandai preset berdasarkan nama saja (mis. setelah rename). */
    fun setName(newName: String?, userPreset: Boolean) {
        name = newName
        isUserPreset = userPreset
    }
}
