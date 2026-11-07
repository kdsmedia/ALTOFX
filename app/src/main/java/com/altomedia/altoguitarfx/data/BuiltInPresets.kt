package com.altomedia.altoguitarfx.data

import com.altomedia.altoguitarfx.effects.EffectType

/**
 * Preset bawaan (BAB 13), mengikuti tabel "Preset awal" pada rancangan, kini
 * dengan model amplifier (Amp) sebagai inti karakter suara bergaya Tonebridge.
 *
 * Nilai parameter adalah titik awal yang wajar; bukan setelan final. Rangkaian
 * efek tiap preset disusun sesuai tabel, lalu efek lain ditambahkan ke urutan
 * dalam keadaan bypass agar seluruh slot tetap lengkap.
 */
object BuiltInPresets {

    // Indeks parameter Amp: 0 voicing, 1 gain, 2 bass, 3 mid, 4 treble,
    // 5 presence, 6 master. Voicing: 0 Clean, 1 Crunch, 2 Lead.
    private fun amp(
        voicing: Float,
        gain: Float,
        bass: Float,
        mid: Float,
        treble: Float,
        presence: Float,
        master: Float
    ) = mapOf(
        0 to voicing, 1 to gain, 2 to bass, 3 to mid,
        4 to treble, 5 to presence, 6 to master
    )

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

    /** Gate → Amp bersih → EQ → Reverb. Bersih dan jernih. */
    private fun clean() = build(
        "CLEAN",
        EffectType.NOISE_GATE to mapOf(0 to -50f, 1 to 150f, 2 to 5f),
        EffectType.AMP to amp(0f, 2f, 3f, -1f, 3f, 1f, 0f),
        EffectType.EQUALIZER to mapOf(0 to 1f, 1 to 0f, 2 to 1.5f),
        EffectType.REVERB to mapOf(0 to 0.35f, 1 to 0.3f, 2 to 0.5f, 3 to 0.18f)
    )

    /** Gate → Compressor ringan → Overdrive → Amp crunch → EQ → Reverb. */
    private fun blues() = build(
        "BLUES",
        EffectType.NOISE_GATE to mapOf(0 to -50f, 1 to 120f, 2 to 5f),
        EffectType.COMPRESSOR to mapOf(0 to -20f, 1 to 3f, 2 to 15f, 3 to 150f, 4 to 4f),
        EffectType.OVERDRIVE to mapOf(0 to 0.45f, 1 to 0.55f, 2 to 0.7f),
        EffectType.AMP to amp(1f, 12f, 3f, 2f, 3f, 1f, -2f),
        EffectType.EQUALIZER to mapOf(0 to 1.5f, 1 to 1f, 2 to 1f),
        EffectType.REVERB to mapOf(0 to 0.4f, 1 to 0.35f, 2 to 0.45f, 3 to 0.22f)
    )

    /** Gate → Amp bersih → EQ → Compressor ringan → Reverb. Akustik. */
    private fun acoustic() = build(
        "ACOUSTIC",
        EffectType.NOISE_GATE to mapOf(0 to -50f, 1 to 150f, 2 to 6f),
        EffectType.AMP to amp(0f, 0f, 2f, -2f, 4f, 2f, 0f),
        EffectType.EQUALIZER to mapOf(0 to 2f, 1 to -1f, 2 to 2.5f),
        EffectType.COMPRESSOR to mapOf(0 to -22f, 1 to 2.5f, 2 to 20f, 3 to 180f, 4 to 3f),
        EffectType.REVERB to mapOf(0 to 0.3f, 1 to 0.3f, 2 to 0.55f, 3 to 0.2f)
    )

    /** Gate → Overdrive → Amp crunch → EQ. Crunch ringan. */
    private fun crunch() = build(
        "CRUNCH",
        EffectType.NOISE_GATE to mapOf(0 to -55f, 1 to 100f, 2 to 4f),
        EffectType.OVERDRIVE to mapOf(0 to 0.6f, 1 to 0.5f, 2 to 0.75f),
        EffectType.AMP to amp(1f, 16f, 2f, 2f, 3f, 1f, -3f),
        EffectType.EQUALIZER to mapOf(0 to 0f, 1 to 2f, 2 to 1f)
    )

    /** Gate → Distortion → Amp lead → EQ → Delay pendek. Rock ritmis. */
    private fun rock() = build(
        "ROCK",
        EffectType.NOISE_GATE to mapOf(0 to -50f, 1 to 90f, 2 to 4f),
        EffectType.DISTORTION to mapOf(0 to 0.6f, 1 to 0.5f, 2 to 0.72f),
        EffectType.AMP to amp(2f, 18f, 3f, 2f, 4f, 2f, -5f),
        EffectType.EQUALIZER to mapOf(0 to 0f, 1 to 1.5f, 2 to 1f),
        EffectType.DELAY to mapOf(0 to 180f, 1 to 0.3f, 2 to 0.2f)
    )

    /** Gate → Distortion → Amp lead → EQ → Chorus tipis. Lebih tebal. */
    private fun hardRock() = build(
        "HARD ROCK",
        EffectType.NOISE_GATE to mapOf(0 to -48f, 1 to 80f, 2 to 3f),
        EffectType.DISTORTION to mapOf(0 to 0.72f, 1 to 0.5f, 2 to 0.72f),
        EffectType.AMP to amp(2f, 20f, 3f, 1f, 4f, 2f, -6f),
        EffectType.EQUALIZER to mapOf(0 to 0.5f, 1 to 1f, 2 to 1f),
        EffectType.CHORUS to mapOf(0 to 0.8f, 1 to 0.25f, 2 to 0.25f)
    )

    /** Gate → Distortion → Amp lead (gain tinggi) → EQ → Compressor. Metal. */
    private fun metal() = build(
        "METAL",
        EffectType.NOISE_GATE to mapOf(0 to -45f, 1 to 60f, 2 to 2f),
        EffectType.DISTORTION to mapOf(0 to 0.9f, 1 to 0.45f, 2 to 0.7f),
        EffectType.AMP to amp(2f, 24f, 4f, 2f, 5f, 3f, -8f),
        EffectType.EQUALIZER to mapOf(0 to 2f, 1 to -3f, 2 to 2f),
        EffectType.COMPRESSOR to mapOf(0 to -18f, 1 to 4f, 2 to 8f, 3 to 100f, 4 to 3f)
    )

    /** Overdrive → Amp lead → Delay → Reverb. Lead dengan sustain dan ruang. */
    private fun leadSolo() = build(
        "LEAD SOLO",
        EffectType.OVERDRIVE to mapOf(0 to 0.7f, 1 to 0.6f, 2 to 0.75f),
        EffectType.AMP to amp(2f, 20f, 2f, 4f, 4f, 2f, -5f),
        EffectType.DELAY to mapOf(0 to 380f, 1 to 0.42f, 2 to 0.3f),
        EffectType.REVERB to mapOf(0 to 0.55f, 1 to 0.5f, 2 to 0.4f, 3 to 0.3f)
    )

    /** Amp bersih → Chorus → Delay → Reverb. Ruang lebar dan atmosferik. */
    private fun ambient() = build(
        "AMBIENT",
        EffectType.AMP to amp(0f, 4f, 3f, 0f, 3f, 2f, 0f),
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
