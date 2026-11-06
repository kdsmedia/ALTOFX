package com.altomedia.altoguitarfx.data

import android.content.Context
import android.content.SharedPreferences
import com.altomedia.altoguitarfx.audio.NativeBridge
import com.altomedia.altoguitarfx.effects.EffectController
import com.altomedia.altoguitarfx.effects.EffectType
import org.json.JSONArray

/**
 * Penyimpanan preset (BAB 13). Preset bawaan dihasilkan di memori; preset
 * pengguna disimpan sebagai JSON di SharedPreferences. Tidak ada data palsu:
 * [apply] benar-benar mengirim urutan, bypass, dan parameter ke mesin DSP,
 * sedangkan [capture] membaca keadaan DSP saat ini.
 */
class PresetRepository(context: Context) {

    private val prefs: SharedPreferences =
        context.applicationContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

    /** Preset bawaan diikuti preset pengguna. */
    fun load(): List<Preset> {
        val user = readUserPresets()
        return BuiltInPresets.all() + user
    }

    /** Menyimpan preset pengguna (menimpa yang bernama sama). */
    fun save(preset: Preset) {
        val user = readUserPresets().filterNot { it.name == preset.name }.toMutableList()
        user.add(preset.copy(builtIn = false))
        writeUserPresets(user)
    }

    /** Menghapus preset pengguna berdasarkan nama. Preset bawaan tidak dihapus. */
    fun delete(name: String) {
        writeUserPresets(readUserPresets().filterNot { it.name == name })
    }

    /** Mengganti nama preset pengguna. */
    fun rename(oldName: String, newName: String) {
        val user = readUserPresets().map {
            if (it.name == oldName) it.copy(name = newName) else it
        }
        writeUserPresets(user)
    }

    /** Menduplikasi preset (bawaan atau pengguna) dengan nama baru. */
    fun duplicate(preset: Preset, newName: String) {
        save(preset.copy(name = newName, builtIn = false))
    }

    /** Mengembalikan daftar preset pengguna ke keadaan kosong. */
    fun clearUserPresets() {
        prefs.edit().remove(KEY_USER_PRESETS).apply()
    }

    /** Menyalin keadaan mesin DSP saat ini menjadi sebuah preset. */
    fun capture(name: String): Preset {
        val count = NativeBridge.effectCount().coerceAtLeast(EffectType.values().size)
        val order = (0 until count).mapNotNull { slot ->
            NativeBridge.effectOrderAt(slot).takeIf { it >= 0 }
        }
        val enabled = mutableSetOf<Int>()
        val params = mutableMapOf<Int, List<Float>>()
        EffectType.values().forEach { type ->
            if (EffectController.isEnabled(type)) enabled.add(type.id)
            params[type.id] = type.params.indices.map { index ->
                EffectController.param(type, index)
            }
        }
        return Preset(name = name, order = order, enabled = enabled, params = params)
    }

    /** Menerapkan preset ke mesin DSP: urutan, bypass, dan semua parameter. */
    fun apply(preset: Preset) {
        EffectType.values().forEach { type ->
            val values = preset.params[type.id] ?: return@forEach
            values.forEachIndexed { index, value ->
                EffectController.setParam(type, index, value)
            }
        }
        // Urutan diterapkan sekaligus dalam satu operasi atomik.
        val target = IntArray(NativeBridge.effectCount())
        for (slot in target.indices) {
            target[slot] = preset.order.getOrElse(slot) { slot }
        }
        NativeBridge.setEffectOrder(target)
        EffectType.values().forEach { type ->
            EffectController.setEnabled(type, preset.enabled.contains(type.id))
        }
    }

    private fun readUserPresets(): MutableList<Preset> {
        val raw = prefs.getString(KEY_USER_PRESETS, null) ?: return mutableListOf()
        return try {
            val array = JSONArray(raw)
            (0 until array.length()).map { Preset.fromJson(array.getJSONObject(it)) }
                .toMutableList()
        } catch (t: Throwable) {
            // Data rusak: mulai dari daftar kosong agar aplikasi tetap berjalan.
            mutableListOf()
        }
    }

    private fun writeUserPresets(presets: List<Preset>) {
        val array = JSONArray()
        presets.forEach { array.put(it.toJson()) }
        prefs.edit().putString(KEY_USER_PRESETS, array.toString()).apply()
    }

    private companion object {
        const val PREFS_NAME = "alto_presets"
        const val KEY_USER_PRESETS = "user_presets"
    }
}
