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
    val params: List<ParamSpec>,
    /**
     * Preset cepat per efek (nama -> nilai parameter per indeks). Dipakai
     * tombol "Preset" pada kotak pedal agar pengguna dapat mengubah karakter
     * efek dengan satu ketukan, seperti preset pada pedal fisik.
     */
    val quickPresets: List<QuickPreset> = emptyList()
) {
    NOISE_GATE(
        id = 0,
        label = "Noise Gate",
        params = listOf(
            ParamSpec("Threshold", -80f, 0f, -45f, "dB"),
            ParamSpec("Release", 5f, 500f, 120f, "ms"),
            ParamSpec("Attack", 1f, 50f, 5f, "ms")
        ),
        quickPresets = listOf(
            QuickPreset("Halus", mapOf(0 to -55f, 1 to 250f, 2 to 8f)),
            QuickPreset("Ketat", mapOf(0 to -38f, 1 to 50f, 2 to 1f)),
            QuickPreset("Sedang", mapOf(0 to -45f, 1 to 120f, 2 to 5f))
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
        ),
        quickPresets = listOf(
            QuickPreset("Ringan", mapOf(0 to -18f, 1 to 3f, 2 to 15f, 3 to 150f, 4 to 4f)),
            QuickPreset("Sedang", mapOf(0 to -22f, 1 to 4f, 2 to 10f, 3 to 120f, 4 to 6f)),
            QuickPreset("Ketat", mapOf(0 to -28f, 1 to 8f, 2 to 5f, 3 to 80f, 4 to 10f))
        )
    ),
    OVERDRIVE(
        id = 2,
        label = "Overdrive",
        params = listOf(
            ParamSpec("Drive", 0f, 1f, 0.4f, ""),
            ParamSpec("Tone", 0f, 1f, 0.5f, ""),
            ParamSpec("Level", 0f, 1f, 0.7f, "")
        ),
        quickPresets = listOf(
            QuickPreset("Warm", mapOf(0 to 0.35f, 1 to 0.45f, 2 to 0.7f)),
            QuickPreset("Blues", mapOf(0 to 0.5f, 1 to 0.55f, 2 to 0.75f)),
            QuickPreset("Boost", mapOf(0 to 0.2f, 1 to 0.6f, 2 to 0.9f))
        )
    ),
    DISTORTION(
        id = 3,
        label = "Distortion",
        params = listOf(
            ParamSpec("Gain", 0f, 1f, 0.5f, ""),
            ParamSpec("Tone", 0f, 1f, 0.5f, ""),
            ParamSpec("Level", 0f, 1f, 0.7f, "")
        ),
        quickPresets = listOf(
            QuickPreset("Rock", mapOf(0 to 0.55f, 1 to 0.5f, 2 to 0.75f)),
            QuickPreset("Hard", mapOf(0 to 0.8f, 1 to 0.45f, 2 to 0.7f)),
            QuickPreset("Metal", mapOf(0 to 0.95f, 1 to 0.4f, 2 to 0.7f))
        )
    ),
    FUZZ(
        id = 4,
        label = "Fuzz",
        params = listOf(
            ParamSpec("Fuzz", 0f, 1f, 0.6f, ""),
            ParamSpec("Tone", 0f, 1f, 0.5f, ""),
            ParamSpec("Volume", 0f, 1f, 0.7f, "")
        ),
        quickPresets = listOf(
            QuickPreset("Klasik", mapOf(0 to 0.6f, 1 to 0.5f, 2 to 0.7f)),
            QuickPreset("Vintage", mapOf(0 to 0.45f, 1 to 0.35f, 2 to 0.75f)),
            QuickPreset("Wild", mapOf(0 to 0.9f, 1 to 0.6f, 2 to 0.65f))
        )
    ),
    AMP(
        id = 5,
        label = "Amp",
        params = listOf(
            ParamSpec("Voicing", 0f, 2f, 1f, ""),
            ParamSpec("Gain", -12f, 30f, 8f, "dB"),
            ParamSpec("Bass", -12f, 12f, 2f, "dB"),
            ParamSpec("Mid", -12f, 12f, 0f, "dB"),
            ParamSpec("Treble", -12f, 12f, 2f, "dB"),
            ParamSpec("Presence", -12f, 12f, 0f, "dB"),
            ParamSpec("Master", -24f, 12f, 0f, "dB")
        ),
        quickPresets = listOf(
            QuickPreset("Clean", mapOf(0 to 0f, 1 to 4f, 2 to 3f, 3 to -1f, 4 to 3f, 5 to 1f, 6 to 0f)),
            QuickPreset("Crunch", mapOf(0 to 1f, 1 to 14f, 2 to 2f, 3 to 1f, 4 to 3f, 5 to 1f, 6 to -2f)),
            QuickPreset("Lead", mapOf(0 to 2f, 1 to 22f, 2 to 4f, 3 to 4f, 4 to 4f, 5 to 2f, 6 to -6f))
        )
    ),
    EQUALIZER(
        id = 6,
        label = "Equalizer",
        params = listOf(
            ParamSpec("Bass", -12f, 12f, 0f, "dB"),
            ParamSpec("Mid", -12f, 12f, 0f, "dB"),
            ParamSpec("Treble", -12f, 12f, 0f, "dB")
        ),
        quickPresets = listOf(
            QuickPreset("Flat", mapOf(0 to 0f, 1 to 0f, 2 to 0f)),
            QuickPreset("Bright", mapOf(0 to -1f, 1 to 0f, 2 to 3f)),
            QuickPreset("Warm", mapOf(0 to 3f, 1 to 1f, 2 to -1f)),
            QuickPreset("Scoop", mapOf(0 to 3f, 1 to -4f, 2 to 3f))
        )
    ),
    CHORUS(
        id = 7,
        label = "Chorus",
        params = listOf(
            ParamSpec("Rate", 0.1f, 8f, 1.0f, "Hz"),
            ParamSpec("Depth", 0f, 1f, 0.4f, ""),
            ParamSpec("Mix", 0f, 1f, 0.5f, "")
        ),
        quickPresets = listOf(
            QuickPreset("Subtil", mapOf(0 to 0.6f, 1 to 0.25f, 2 to 0.35f)),
            QuickPreset("Klasik", mapOf(0 to 1.0f, 1 to 0.45f, 2 to 0.5f)),
            QuickPreset("Lebar", mapOf(0 to 0.4f, 1 to 0.7f, 2 to 0.6f))
        )
    ),
    FLANGER(
        id = 8,
        label = "Flanger",
        params = listOf(
            ParamSpec("Rate", 0.05f, 5f, 0.5f, "Hz"),
            ParamSpec("Depth", 0f, 1f, 0.5f, ""),
            ParamSpec("Feedback", 0f, 0.95f, 0.5f, ""),
            ParamSpec("Mix", 0f, 1f, 0.5f, "")
        ),
        quickPresets = listOf(
            QuickPreset("Subtil", mapOf(0 to 0.5f, 1 to 0.4f, 2 to 0.3f, 3 to 0.4f)),
            QuickPreset("Jet", mapOf(0 to 0.3f, 1 to 0.8f, 2 to 0.7f, 3 to 0.5f)),
            QuickPreset("Sweep", mapOf(0 to 0.15f, 1 to 0.9f, 2 to 0.6f, 3 to 0.55f))
        )
    ),
    PHASER(
        id = 9,
        label = "Phaser",
        params = listOf(
            ParamSpec("Rate", 0.05f, 5f, 0.5f, "Hz"),
            ParamSpec("Depth", 0f, 1f, 0.5f, ""),
            ParamSpec("Feedback", 0f, 0.95f, 0.5f, ""),
            ParamSpec("Mix", 0f, 1f, 0.5f, "")
        ),
        quickPresets = listOf(
            QuickPreset("Subtil", mapOf(0 to 0.4f, 1 to 0.4f, 2 to 0.3f, 3 to 0.4f)),
            QuickPreset("Klasik", mapOf(0 to 0.5f, 1 to 0.6f, 2 to 0.5f, 3 to 0.5f)),
            QuickPreset("Dalam", mapOf(0 to 0.25f, 1 to 0.8f, 2 to 0.7f, 3 to 0.6f))
        )
    ),
    TREMOLO(
        id = 10,
        label = "Tremolo",
        params = listOf(
            ParamSpec("Rate", 0.5f, 15f, 5f, "Hz"),
            ParamSpec("Depth", 0f, 1f, 0.5f, "")
        ),
        quickPresets = listOf(
            QuickPreset("Subtil", mapOf(0 to 4f, 1 to 0.3f)),
            QuickPreset("Klasik", mapOf(0 to 5f, 1 to 0.5f)),
            QuickPreset("Dalam", mapOf(0 to 6f, 1 to 0.8f))
        )
    ),
    DELAY(
        id = 11,
        label = "Delay",
        params = listOf(
            ParamSpec("Time", 20f, 1000f, 300f, "ms"),
            ParamSpec("Feedback", 0f, 0.95f, 0.4f, ""),
            ParamSpec("Mix", 0f, 1f, 0.3f, "")
        ),
        quickPresets = listOf(
            QuickPreset("Slap", mapOf(0 to 110f, 1 to 0.15f, 2 to 0.25f)),
            QuickPreset("Klasik", mapOf(0 to 300f, 1 to 0.4f, 2 to 0.3f)),
            QuickPreset("Panjang", mapOf(0 to 450f, 1 to 0.5f, 2 to 0.35f))
        )
    ),
    REVERB(
        id = 12,
        label = "Reverb",
        params = listOf(
            ParamSpec("Size", 0f, 1f, 0.5f, ""),
            ParamSpec("Decay", 0f, 1f, 0.5f, ""),
            ParamSpec("Damping", 0f, 1f, 0.5f, ""),
            ParamSpec("Mix", 0f, 1f, 0.3f, "")
        ),
        quickPresets = listOf(
            QuickPreset("Ruang", mapOf(0 to 0.4f, 1 to 0.4f, 2 to 0.5f, 3 to 0.2f)),
            QuickPreset("Hall", mapOf(0 to 0.7f, 1 to 0.6f, 2 to 0.4f, 3 to 0.3f)),
            QuickPreset("Ambien", mapOf(0 to 0.9f, 1 to 0.85f, 2 to 0.3f, 3 to 0.45f))
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

/** Preset cepat satu efek: nama tampil dan nilai parameter per indeks. */
data class QuickPreset(
    val name: String,
    val values: Map<Int, Float>
)
