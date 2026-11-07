package com.altomedia.altoguitarfx.data

import com.altomedia.altoguitarfx.effects.EffectType

/**
 * Perpustakaan "tone" bergaya Tonebridge: setiap tone adalah preset siap pakai
 * yang dinamai sesuai lagu/artis, dibangun di atas salah satu preset bawaan
 * dengan penyesuaian amplifier (voicing/gain/tone) agar karakternya khas.
 *
 * Ini bukan sampel audio dari rekaman asli; tone adalah hasil rancangan
 * parameter DSP (model amplifier + efek) untuk mendekati karakter tersebut.
 */
object ToneLibrary {

    /**
     * Satu tone terkurasi.
     *
     * @param base nama preset bawaan yang menjadi dasar (lihat [BuiltInPresets]).
     * @param ampOverrides nilai parameter Amp yang ditimpa (indeks sesuai
     *        [EffectType.AMP]: 0 voicing, 1 gain, 2 bass, 3 mid, 4 treble,
     *        5 presence, 6 master).
     */
    data class Tone(
        val name: String,
        val artist: String,
        val style: String,
        val rating: Int,
        val base: String,
        val ampOverrides: Map<Int, Float> = emptyMap()
    )

    fun all(): List<Tone> = listOf(
        Tone("Back in Black", "AC/DC", "Hard Rock", 5, "ROCK", mapOf(1 to 20f, 3 to 2f)),
        Tone("Thunderstruck", "AC/DC", "Hard Rock", 5, "HARD ROCK", mapOf(1 to 22f)),
        Tone("Smoke on the Water", "Deep Purple", "Rock", 5, "ROCK", mapOf(1 to 18f, 4 to 4f)),
        Tone("Highway Star", "Deep Purple", "Rock", 4, "LEAD SOLO", mapOf(1 to 22f)),
        Tone("Sweet Child O' Mine", "Guns N' Roses", "Rock", 5, "LEAD SOLO", mapOf(1 to 20f, 4 to 5f)),
        Tone("Welcome to the Jungle", "Guns N' Roses", "Hard Rock", 5, "HARD ROCK", mapOf(1 to 24f)),
        Tone("Purple Haze", "Jimi Hendrix", "Psychedelic", 5, "CRUNCH", mapOf(1 to 18f, 4 to 5f)),
        Tone("Little Wing", "Jimi Hendrix", "Blues Rock", 4, "BLUES", mapOf(1 to 14f, 5 to 3f)),
        Tone("Whole Lotta Love", "Led Zeppelin", "Rock", 4, "HARD ROCK", mapOf(1 to 21f)),
        Tone("Stairway to Heaven", "Led Zeppelin", "Classic Rock", 5, "LEAD SOLO", mapOf(1 to 18f, 4 to 4f)),
        Tone("Enter Sandman", "Metallica", "Metal", 5, "METAL", mapOf(1 to 26f, 3 to 3f)),
        Tone("Master of Puppets", "Metallica", "Thrash Metal", 5, "METAL", mapOf(1 to 28f, 4 to 6f)),
        Tone("Nothing Else Matters", "Metallica", "Ballad", 4, "CLEAN", mapOf(0 to 0f, 1 to 4f)),
        Tone("Walk", "Pantera", "Groove Metal", 4, "METAL", mapOf(1 to 26f, 2 to 3f, 3 to -4f)),
        Tone("Everlong", "Foo Fighters", "Alt Rock", 4, "HARD ROCK", mapOf(1 to 20f)),
        Tone("Come As You Are", "Nirvana", "Grunge", 4, "CRUNCH", mapOf(1 to 16f, 3 to -1f)),
        Tone("Pride and Joy", "Stevie Ray Vaughan", "Blues", 5, "BLUES", mapOf(1 to 16f, 5 to 3f)),
        Tone("The Thrill Is Gone", "B.B. King", "Blues", 4, "BLUES", mapOf(1 to 12f, 3 to 2f)),
        Tone("Still Got the Blues", "Gary Moore", "Blues", 5, "LEAD SOLO", mapOf(1 to 20f, 4 to 4f)),
        Tone("Hotel California", "Eagles", "Classic Rock", 5, "LEAD SOLO", mapOf(1 to 18f, 4 to 3f)),
        Tone("Sultans of Swing", "Dire Straits", "Classic Rock", 4, "CLEAN", mapOf(0 to 0f, 1 to 6f, 5 to 3f)),
        Tone("Wish You Were Here", "Pink Floyd", "Progressive", 5, "AMBIENT", mapOf(1 to 6f)),
        Tone("Comfortably Numb", "Pink Floyd", "Progressive", 5, "LEAD SOLO", mapOf(1 to 22f, 4 to 5f)),
        Tone("Wonderwall", "Oasis", "Britpop", 4, "ACOUSTIC", mapOf(0 to 0f, 1 to 2f)),
        Tone("Hallelujah", "Jeff Buckley", "Ballad", 4, "CLEAN", mapOf(0 to 0f, 1 to 3f, 5 to 4f)),
        Tone("Chasing Cars", "Snow Patrol", "Indie", 4, "AMBIENT", mapOf(1 to 5f, 4 to 3f))
    )

    /** Membangun preset siap pakai dari sebuah tone. */
    fun preset(tone: Tone): Preset {
        val base = BuiltInPresets.all().firstOrNull { it.name == tone.base }
            ?: BuiltInPresets.all().first()
        val ampParams = base.params[EffectType.AMP.id].orEmpty().toMutableList()
        tone.ampOverrides.forEach { (index, value) ->
            if (index in ampParams.indices) ampParams[index] = value
        }
        val params = base.params.toMutableMap().also { it[EffectType.AMP.id] = ampParams }
        return base.copy(name = tone.name, params = params, builtIn = true)
    }
}
