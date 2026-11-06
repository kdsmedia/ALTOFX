package com.altomedia.altoguitarfx.audio

import android.content.Context
import android.media.AudioAttributes
import android.media.AudioFocusRequest
import android.media.AudioManager
import android.os.Build

/**
 * Pengelola audio focus (BAB 15).
 *
 * Meminta audio focus untuk pemakaian jangka panjang (gitar live) dan
 * memberi tahu pemanggil ketika focus hilang atau diperoleh kembali, sehingga
 * stream dapat dijeda/dipulihkan dengan benar. Tanpa ini, panggilan masuk
 * atau aplikasi lain dapat membuat audio terhenti tanpa penanganan.
 */
class AudioFocusController(context: Context) {

    /** Reaksi yang harus diambil pemanggil terhadap perubahan focus. */
    enum class Event { GAIN, LOSS, LOSS_TRANSIENT, DUCK, UNDUCK }

    private val audioManager =
        context.applicationContext.getSystemService(Context.AUDIO_SERVICE) as AudioManager

    private var listener: ((Event) -> Unit)? = null
    private var focusRequest: AudioFocusRequest? = null
    private var hasFocus = false

    private val focusListener = AudioManager.OnAudioFocusChangeListener { change ->
        when (change) {
            AudioManager.AUDIOFOCUS_GAIN -> {
                hasFocus = true
                listener?.invoke(Event.UNDUCK)
                listener?.invoke(Event.GAIN)
            }
            AudioManager.AUDIOFOCUS_LOSS -> {
                hasFocus = false
                listener?.invoke(Event.LOSS)
            }
            AudioManager.AUDIOFOCUS_LOSS_TRANSIENT -> {
                hasFocus = false
                listener?.invoke(Event.LOSS_TRANSIENT)
            }
            AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK -> {
                // Focus sementara hilang tetapi kita boleh tetap berbunyi dengan
                // level lebih rendah (ducking), bukan dijeda.
                listener?.invoke(Event.DUCK)
            }
        }
    }

    /** Memasang callback perubahan focus. */
    fun setListener(callback: (Event) -> Unit) {
        listener = callback
    }

    /** Meminta audio focus. Mengembalikan true bila berhasil. */
    fun request(): Boolean {
        val attributes = AudioAttributes.Builder()
            .setUsage(AudioAttributes.USAGE_MEDIA)
            .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
            .build()
        val result = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val request = AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
                .setAudioAttributes(attributes)
                .setAcceptsDelayedFocusGain(false)
                .setOnAudioFocusChangeListener(focusListener)
                .build()
            focusRequest = request
            audioManager.requestAudioFocus(request)
        } else {
            @Suppress("DEPRECATION")
            audioManager.requestAudioFocus(
                focusListener,
                AudioManager.STREAM_MUSIC,
                AudioManager.AUDIOFOCUS_GAIN
            )
        }
        hasFocus = result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED
        return hasFocus
    }

    /** Melepas audio focus. */
    fun abandon() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            focusRequest?.let { audioManager.abandonAudioFocusRequest(it) }
        } else {
            @Suppress("DEPRECATION")
            audioManager.abandonAudioFocus(focusListener)
        }
        focusRequest = null
        hasFocus = false
    }

    fun hasFocus(): Boolean = hasFocus
}
