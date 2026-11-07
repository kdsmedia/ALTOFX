package com.altomedia.altoguitarfx.data

import com.altomedia.altoguitarfx.effects.EffectType

/**
 * Preset bawaan (BAB 13), mengikuti tabel "Preset awal" pada rancangan.
 *
 * Nilai parameter adalah titik awal yang wajar; bukan setelan final. Rangkaian
 * efek tiap preset disusun sesuai tabel, lalu efek lain ditambahkan ke urutan
 * dalam keadaan bypass agar seluruh slot tetap lengkap.
 */
object BuiltInPresets {

    fun all(): List<Preset> = listOf(
        clean(),
        blues(),
        acoustic(),
        crunch(),
        rock(),
        hardRock(),
        metal(),
        leadSolo(),
        ambient()
    )

    /** Gate ringan → EQ → Reverb. Bersih dan jernih. */
    private fun clean() = build(
        "CLEAN",
        EffectType.NOISE_GATE to mapOf(0 to -50f, 1 to 150f, 2 to 5f),
        EffectType.EQUALIZER to mapOf(0 to 1f, 1 to 0f, 2 to 1.5f),
        EffectType.REVERB to mapOf(0 to 0.35f, 1 to 0.3f, 2 to 0.5f, 3 to 0.18f)
    )

    /** Gate → Compressor ringan → Overdrive → EQ → Reverb. Hangat dan dinamis. */
    private fun blues() = build(
        "BLUES",
        EffectType.NOISE_GATE to mapOf(0 to -50f, 1 to 120f, 2 to 5f),
        EffectType.COMPRESSOR to mapOf(0 to -20f, 1 to 3f, 2 to 15f, 3 to 150f, 4 to 4f),
        EffectType.OVERDRIVE to mapOf(0 to 0.45f, 1 to 0.55f, 2 to 0.7f),
        EffectType.EQUALIZER to mapOf(0 to 1.5f, 1 to 1f, 2 to 1f),
        EffectType.REVERB to mapOf(0 to 0.4f, 1 to 0.35f, 2 to 0.45f, 3 to 0.22f)
    )

    /** Gate → EQ → Compressor ringan → Reverb. Menonjolkan karakter input. */
    private fun acoustic() = build(
        "ACOUSTIC",
        EffectType.NOISE_GATE to mapOf(0 to -50f, 1 to 150f, 2 to 6f),
        EffectType.EQUALIZER to mapOf(0 to 2f, 1 to -1f, 2 to 2.5f),
        EffectType.COMPRESSOR to mapOf(0 to -22f, 1 to 2.5f, 2 to 20f, 3 to 180f, 4 to 3f),
        EffectType.REVERB to mapOf(0 to 0.3f, 1 to 0.3f, 2 to 0.55f, 3 to 0.2f)
    )

    /** Gate → Overdrive → EQ. Crunch ringan. */
    private fun crunch() = build(
        "CRUNCH",
        EffectType.NOISE_GATE to mapOf(0 to -55f, 1 to 100f, 2 to 4f),
        EffectType.OVERDRIVE to mapOf(0 to 0.6f, 1 to 0.5f, 2 to 0.75f),
        EffectType.EQUALIZER to mapOf(0 to 0f, 1 to 2f, 2 to 1f)
    )

    /** Gate → Distortion → EQ → Delay pendek. Rock ritmis. */
    private fun rock() = build(
        "ROCK",
        EffectType.NOISE_GATE to mapOf(0 to -50f, 1 to 90f, 2 to 4f),
        EffectType.DISTORTION to mapOf(0 to 0.6f, 1 to 0.5f, 2 to 0.72f),
        EffectType.EQUALIZER to mapOf(0 to 0f, 1 to 1.5f, 2 to 1f),
        EffectType.DELAY to mapOf(0 to 180f, 1 to 0.3f, 2 to 0.2f)
    )

    /** Gate → Distortion → EQ → Chorus tipis. Lebih tebal. */
    private fun hardRock() = build(
        "HARD ROCK",
        EffectType.NOISE_GATE to mapOf(0 to -48f, 1 to 80f, 2 to 3f),
        EffectType.DISTORTION to mapOf(0 to 0.72f, 1 to 0.5f, 2 to 0.72f),
        EffectType.EQUALIZER to mapOf(0 to 0.5f, 1 to 1f, 2 to 1f),
        EffectType.CHORUS to mapOf(0 to 0.8f, 1 to 0.25f, 2 to 0.25f)
    )

    /** Gate → Distortion → EQ → Compressor. Gain tinggi. */
    private fun metal() = build(
        "METAL",
        EffectType.NOISE_GATE to mapOf(0 to -45f, 1 to 60f, 2 to 2f),
        EffectType.DISTORTION to mapOf(0 to 0.9f, 1 to 0.45f, 2 to 0.7f),
        EffectType.EQUALIZER to mapOf(0 to 2f, 1 to -3f, 2 to 2f),
        EffectType.COMPRESSOR to mapOf(0 to -18f, 1 to 4f, 2 to 8f, 3 to 100f, 4 to 3f)
    )

    /** Overdrive → Delay → Reverb. Lead dengan sustain dan ruang. */
    private fun leadSolo() = build(
        "LEAD SOLO",
        EffectType.OVERDRIVE to mapOf(0 to 0.7f, 1 to 0.6f, 2 to 0.75f),
        EffectType.DELAY to mapOf(0 to 380f, 1 to 0.42f, 2 to 0.3f),
        EffectType.REVERB to mapOf(0 to 0.55f, 1 to 0.5f, 2 to 0.4f, 3 to 0.3f)
    )

    /** Chorus → Delay → Reverb. Ruang lebar dan atmosferik. */
    private fun ambient() = build(
        "AMBIENT",
        EffectType.CHORUS to mapOf(0 to 0.6f, 1 to 0.6f, 2 to 0.5f),
        EffectType.DELAY to mapOf(0 to 500f, 1 to 0.5f, 2 to 0.35f),
        EffectType.REVERB to mapOf(0 to 0.85f, 1 to 0.8f, 2 to 0.35f, 3 to 0.45f)
    )

    /**
     * Menyusun preset dari rangkaian efek yang aktif. Efek pada [steps]
     * ditempatkan lebih dahulu sesuai urutan tabel; sisanya mengikuti dalam
     * keadaan bypass.
     */
    private fun build(
        name: String,
        vararg steps: Pair<EffectType, Map<Int, Float>>
    ): Preset {
        val activeIds = steps.map { it.first.id }
        val order = activeIds + EffectType.values().map { it.id }.filter { it !in activeIds }
        val params = EffectType.values().associate { type ->
            val overrides = steps.firstOrNull { it.first == type }?.second ?: emptyMap()
            type.id to type.params.mapIndexed { index, spec ->
                overrides[index] ?: spec.default
            }
        }
        return Preset(
            name = name,
            order = order,
            enabled = activeIds.toSet(),
            params = params,
            builtIn = true
        )
    }
}
