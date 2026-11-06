package com.altomedia.altoguitarfx.effects

import com.altomedia.altoguitarfx.audio.NativeBridge

/**
 * Pengendali efek (BAB 7+).
 *
 * Menjembatani UI dengan rangkaian efek native. Nilai parameter disimpan di
 * sisi native sebagai atomik, jadi pengaturan dari UI tidak pernah mengunci
 * audio callback.
 */
object EffectController {

    fun setEnabled(type: EffectType, enabled: Boolean) {
        NativeBridge.setEffectEnabled(type.id, enabled)
    }

    fun isEnabled(type: EffectType): Boolean = NativeBridge.isEffectEnabled(type.id)

    fun setParam(type: EffectType, index: Int, value: Float) {
        NativeBridge.setEffectParam(type.id, index, value)
    }

    fun param(type: EffectType, index: Int): Float =
        NativeBridge.effectParam(type.id, index)

    /** Menyetel seluruh parameter ke nilai bawaan (dipakai saat reset). */
    fun applyDefaults(type: EffectType) {
        type.params.forEachIndexed { index, spec ->
            setParam(type, index, spec.default)
        }
    }
}
