package com.altomedia.altoguitarfx.effects

/**
 * Jenis efek dengan urutan tetap sesuai rancangan (PDF bagian 3 & 7).
 *
 * Nilai [id] harus sama dengan indeks enum EffectType pada sisi C++
 * (app/src/main/cpp/Effects.h). Jangan mengubah urutannya tanpa mengubah
 * keduanya.
 */
enum class EffectType(
    val id: Int,
    val label: String,
    /** Rentang dan satuan parameter, sejajar dengan urutan di sisi C++. */
    val params: List<ParamSpec>
) {
    NOISE_GATE(
        id = 0,
        label = "Noise Gate",
        params = listOf(
            ParamSpec("Threshold", -80f, 0f, -60f, "dB"),
            ParamSpec("Release", 5f, 500f, 120f, "ms"),
            ParamSpec("Attack", 1f, 50f, 5f, "ms")
        )
    ),
    COMPRESSOR(
        id = 1,
        label = "Compressor",
        params = listOf(
            ParamSpec("Threshold", -60f, 0f, -18f, "dB"),
            ParamSpec("Ratio", 1f, 20f, 4f, ":1"),
            ParamSpec("Attack", 0.5f, 200f, 10f, "ms"),
            ParamSpec("Release", 10f, 1000f, 120f, "ms"),
            ParamSpec("Makeup", 0f, 24f, 6f, "dB")
        )
    ),
    OVERDRIVE(
        id = 2,
        label = "Overdrive",
        params = listOf(
            ParamSpec("Drive", 0f, 1f, 0.4f, ""),
            ParamSpec("Tone", 0f, 1f, 0.5f, ""),
            ParamSpec("Level", 0f, 1f, 0.7f, "")
        )
    ),
    DISTORTION(
        id = 3,
        label = "Distortion",
        params = listOf(
            ParamSpec("Gain", 0f, 1f, 0.5f, ""),
            ParamSpec("Tone", 0f, 1f, 0.5f, ""),
            ParamSpec("Level", 0f, 1f, 0.7f, "")
        )
    ),
    FUZZ(
        id = 4,
        label = "Fuzz",
        params = listOf(
            ParamSpec("Fuzz", 0f, 1f, 0.6f, ""),
            ParamSpec("Tone", 0f, 1f, 0.5f, ""),
            ParamSpec("Volume", 0f, 1f, 0.7f, "")
        )
    ),
    EQUALIZER(
        id = 5,
        label = "Equalizer",
        params = listOf(
            ParamSpec("Bass", -12f, 12f, 0f, "dB"),
            ParamSpec("Mid", -12f, 12f, 0f, "dB"),
            ParamSpec("Treble", -12f, 12f, 0f, "dB")
        )
    ),
    CHORUS(
        id = 6,
        label = "Chorus",
        params = listOf(
            ParamSpec("Rate", 0.1f, 8f, 1.0f, "Hz"),
            ParamSpec("Depth", 0f, 1f, 0.4f, ""),
            ParamSpec("Mix", 0f, 1f, 0.5f, "")
        )
    ),
    FLANGER(
        id = 7,
        label = "Flanger",
        params = listOf(
            ParamSpec("Rate", 0.05f, 5f, 0.5f, "Hz"),
            ParamSpec("Depth", 0f, 1f, 0.5f, ""),
            ParamSpec("Feedback", 0f, 0.95f, 0.5f, ""),
            ParamSpec("Mix", 0f, 1f, 0.5f, "")
        )
    ),
    PHASER(
        id = 8,
        label = "Phaser",
        params = listOf(
            ParamSpec("Rate", 0.05f, 5f, 0.5f, "Hz"),
            ParamSpec("Depth", 0f, 1f, 0.5f, ""),
            ParamSpec("Feedback", 0f, 0.95f, 0.5f, ""),
            ParamSpec("Mix", 0f, 1f, 0.5f, "")
        )
    ),
    TREMOLO(
        id = 9,
        label = "Tremolo",
        params = listOf(
            ParamSpec("Rate", 0.5f, 15f, 5f, "Hz"),
            ParamSpec("Depth", 0f, 1f, 0.5f, "")
        )
    ),
    DELAY(
        id = 10,
        label = "Delay",
        params = listOf(
            ParamSpec("Time", 20f, 1000f, 300f, "ms"),
            ParamSpec("Feedback", 0f, 0.95f, 0.4f, ""),
            ParamSpec("Mix", 0f, 1f, 0.3f, "")
        )
    ),
    REVERB(
        id = 11,
        label = "Reverb",
        params = listOf(
            ParamSpec("Size", 0f, 1f, 0.5f, ""),
            ParamSpec("Decay", 0f, 1f, 0.5f, ""),
            ParamSpec("Damping", 0f, 1f, 0.5f, ""),
            ParamSpec("Mix", 0f, 1f, 0.3f, "")
        )
    )
}

/** Spesifikasi satu parameter efek untuk membangun UI kontrol. */
data class ParamSpec(
    val label: String,
    val min: Float,
    val max: Float,
    val default: Float,
    val unit: String
)
