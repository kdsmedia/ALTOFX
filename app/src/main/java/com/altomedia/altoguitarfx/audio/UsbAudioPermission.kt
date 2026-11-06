package com.altomedia.altoguitarfx.audio

import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbConstants
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import android.os.Build
import androidx.core.content.ContextCompat

/**
 * Izin dan deteksi USB host untuk USB Audio Interface (mis. iRig) — blueprint
 * BAB 10 ('periksa dukungan perangkat dan permission USB saat perangkat
 * terpasang').
 *
 * USB Audio Class tidak memerlukan pembukaan endpoint: Android mengeksposnya
 * sebagai perangkat audio lewat AudioManager. Namun bila sistem meminta izin
 * akses ke perangkat USB (mis. interface dengan kontrol vendor), kelas ini
 * meminta izin tersebut secara nyata lewat ACTION_USB_PERMISSION dan
 * melaporkan hasilnya.
 */
object UsbAudioPermission {

    const val ACTION_USB_PERMISSION = "com.altomedia.altoguitarfx.USB_PERMISSION"

    /** Daftar perangkat USB audio yang saat ini terpasang. */
    fun audioDevices(context: Context): List<UsbDevice> {
        val manager = context.getSystemService(Context.USB_SERVICE) as? UsbManager
            ?: return emptyList()
        return manager.deviceList.values.filter { isAudioClass(it) }
    }

    /** true bila perangkat USB tersebut adalah kelas audio (interface gitar). */
    fun isAudioClass(device: UsbDevice): Boolean {
        for (index in 0 until device.interfaceCount) {
            val iface = device.getInterface(index)
            if (iface.interfaceClass == UsbConstants.USB_CLASS_AUDIO) return true
        }
        return false
    }

    /** true bila aplikasi sudah punya izin untuk perangkat ini. */
    fun hasPermission(context: Context, device: UsbDevice): Boolean {
        val manager = context.getSystemService(Context.USB_SERVICE) as? UsbManager
            ?: return false
        return manager.hasPermission(device)
    }

    /**
     * Meminta izin USB untuk [device] bila belum ada. [onResult] dipanggil pada
     * main thread dengan hasil sebenarnya dari sistem. Bila izin sudah ada,
     * [onResult] langsung dipanggil true.
     */
    fun request(
        context: Context,
        device: UsbDevice,
        onResult: (granted: Boolean) -> Unit
    ) {
        val manager = context.getSystemService(Context.USB_SERVICE) as? UsbManager
            ?: return onResult(false)
        if (manager.hasPermission(device)) {
            onResult(true)
            return
        }

        val receiver = object : BroadcastReceiver() {
            override fun onReceive(ctx: Context, intent: Intent) {
                if (intent.action != ACTION_USB_PERMISSION) return
                val granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
                runCatching { ctx.unregisterReceiver(this) }
                onResult(granted)
            }
        }
        val flags = if (Build.VERSION.SDK_INT >= 33) {
            ContextCompat.RECEIVER_NOT_EXPORTED
        } else {
            0
        }
        ContextCompat.registerReceiver(
            context,
            receiver,
            IntentFilter(ACTION_USB_PERMISSION),
            flags
        )

        val permissionIntent = PendingIntent.getBroadcast(
            context,
            0,
            Intent(ACTION_USB_PERMISSION).setPackage(context.packageName),
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        )
        manager.requestPermission(device, permissionIntent)
    }
}
