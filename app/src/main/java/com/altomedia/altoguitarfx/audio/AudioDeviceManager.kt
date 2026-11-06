package com.altomedia.altoguitarfx.audio

import android.content.Context
import android.media.AudioDeviceCallback
import android.media.AudioDeviceInfo
import android.media.AudioManager
import android.os.Handler
import android.os.Looper

/**
 * Manajemen perangkat audio (BAB 5).
 *
 * Membaca daftar perangkat input/output yang benar-benar terpasang pada
 * perangkat Android lewat AudioManager, dan memantau perubahan koneksi
 * (mis. iRig / USB audio interface dicabut atau dicolok) melalui
 * AudioDeviceCallback. Tidak ada data dummy: seluruh daftar berasal dari
 * sistem.
 */
class AudioDeviceManager(context: Context) {

    private val audioManager =
        context.applicationContext.getSystemService(Context.AUDIO_SERVICE) as AudioManager

    /** Deskripsi ringkas satu perangkat audio. */
    data class Device(
        val id: Int,
        val name: String,
        val type: Int,
        val isUsb: Boolean,
        val sampleRates: List<Int>,
        val channelCounts: List<Int>
    ) {
        /** Label tipe perangkat yang mudah dibaca. */
        val typeLabel: String
            get() = when (type) {
                AudioDeviceInfo.TYPE_BUILTIN_MIC -> "Mikrofon internal"
                AudioDeviceInfo.TYPE_BUILTIN_SPEAKER -> "Speaker internal"
                AudioDeviceInfo.TYPE_BUILTIN_EARPIECE -> "Earpiece"
                AudioDeviceInfo.TYPE_WIRED_HEADSET -> "Headset kabel"
                AudioDeviceInfo.TYPE_WIRED_HEADPHONES -> "Headphone kabel"
                AudioDeviceInfo.TYPE_USB_DEVICE -> "USB audio"
                AudioDeviceInfo.TYPE_USB_HEADSET -> "USB headset"
                AudioDeviceInfo.TYPE_USB_ACCESSORY -> "USB aksesori"
                AudioDeviceInfo.TYPE_BLUETOOTH_SCO -> "Bluetooth SCO"
                AudioDeviceInfo.TYPE_BLUETOOTH_A2DP -> "Bluetooth A2DP"
                AudioDeviceInfo.TYPE_TELEPHONY -> "Telepon"
                else -> "Tipe $type"
            }
    }

    /** Perangkat input (sumber sinyal) yang tersedia saat ini. */
    fun inputDevices(): List<Device> = audioManager
        .getDevices(AudioManager.GET_DEVICES_INPUTS)
        .map { it.toDevice() }

    /** Perangkat output (tujuan sinyal) yang tersedia saat ini. */
    fun outputDevices(): List<Device> = audioManager
        .getDevices(AudioManager.GET_DEVICES_OUTPUTS)
        .map { it.toDevice() }

    /** true bila ada audio interface / headset USB yang terpasang. */
    fun hasUsbDevice(): Boolean =
        (inputDevices() + outputDevices()).any { it.isUsb }

    /**
     * true bila headphone (kabel atau USB) terpasang. Dipakai untuk
     * menyarankan headphone guna mencegah feedback (blueprint bagian 6).
     */
    fun hasHeadphone(): Boolean = outputDevices().any {
        it.type == AudioDeviceInfo.TYPE_WIRED_HEADPHONES ||
            it.type == AudioDeviceInfo.TYPE_WIRED_HEADSET ||
            it.type == AudioDeviceInfo.TYPE_USB_HEADSET ||
            it.type == AudioDeviceInfo.TYPE_USB_DEVICE ||
            it.type == AudioDeviceInfo.TYPE_USB_ACCESSORY
    }

    /**
     * Memantau perubahan koneksi perangkat. [onChanged] dipanggil pada main
     * thread setiap perangkat ditambah/dilepas. Kembalikan callback yang sama
     * untuk dipakai pada [stopObserving].
     */
    fun observe(onChanged: () -> Unit): AudioDeviceCallback {
        val callback = object : AudioDeviceCallback() {
            override fun onAudioDevicesAdded(addedDevices: Array<out AudioDeviceInfo>) = onChanged()
            override fun onAudioDevicesRemoved(removedDevices: Array<out AudioDeviceInfo>) = onChanged()
        }
        audioManager.registerAudioDeviceCallback(callback, Handler(Looper.getMainLooper()))
        return callback
    }

    fun stopObserving(callback: AudioDeviceCallback) {
        audioManager.unregisterAudioDeviceCallback(callback)
    }

    private fun AudioDeviceInfo.toDevice() = Device(
        id = id,
        name = productName?.toString().orEmpty(),
        type = type,
        isUsb = type == AudioDeviceInfo.TYPE_USB_DEVICE ||
            type == AudioDeviceInfo.TYPE_USB_HEADSET ||
            type == AudioDeviceInfo.TYPE_USB_ACCESSORY,
        sampleRates = sampleRates.toList(),
        channelCounts = channelCounts.toList()
    )
}
