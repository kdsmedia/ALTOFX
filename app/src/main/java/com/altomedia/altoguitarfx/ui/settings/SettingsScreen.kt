package com.altomedia.altoguitarfx.ui.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.RadioButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.altomedia.altoguitarfx.audio.AudioController
import com.altomedia.altoguitarfx.audio.AudioDeviceManager
import com.altomedia.altoguitarfx.audio.NativeBridge
import com.altomedia.altoguitarfx.ui.components.PanelCard
import com.altomedia.altoguitarfx.ui.components.ScreenScaffold
import com.altomedia.altoguitarfx.ui.theme.Accent
import com.altomedia.altoguitarfx.ui.theme.TextSecondary

/**
 * Settings (BAB 5). Menampilkan perangkat input/output nyata dari sistem dan
 * memungkinkan pemilihan sumber sinyal (mikrofon internal atau USB seperti
 * iRig). Daftar diperbarui otomatis saat perangkat dicabut/dicolok.
 */
@Composable
fun SettingsScreen() {
    val context = LocalContext.current
    val manager = remember { AudioDeviceManager(context) }

    var inputs by remember { mutableStateOf(manager.inputDevices()) }
    var outputs by remember { mutableStateOf(manager.outputDevices()) }
    var selectedInput by remember { mutableStateOf(AudioController.selectedInputId()) }
    var selectedOutput by remember { mutableStateOf(AudioController.selectedOutputId()) }

    DisposableEffect(Unit) {
        val callback = manager.observe {
            inputs = manager.inputDevices()
            outputs = manager.outputDevices()
        }
        onDispose { manager.stopObserving(callback) }
    }

    ScreenScaffold(
        title = "Settings",
        subtitle = "Perangkat dan performa audio"
    ) {
        PanelCard(title = "Sumber Input") {
            if (inputs.isEmpty()) {
                Text(
                    text = "Tidak ada perangkat input terdeteksi.",
                    color = TextSecondary,
                    fontSize = 13.sp
                )
            }
            inputs.forEach { device ->
                DeviceRow(
                    label = "${device.name.ifEmpty { "Input" }} • ${device.typeLabel}",
                    detail = deviceDetail(device.sampleRates, device.channelCounts),
                    selected = selectedInput == device.id,
                    onClick = {
                        selectedInput = device.id
                        AudioController.selectInput(device.id)
                    }
                )
            }
        }

        PanelCard(
            title = "Tujuan Output",
            modifier = Modifier.padding(top = 12.dp)
        ) {
            if (outputs.isEmpty()) {
                Text(
                    text = "Tidak ada perangkat output terdeteksi.",
                    color = TextSecondary,
                    fontSize = 13.sp
                )
            }
            outputs.forEach { device ->
                DeviceRow(
                    label = "${device.name.ifEmpty { "Output" }} • ${device.typeLabel}",
                    detail = deviceDetail(device.sampleRates, device.channelCounts),
                    selected = selectedOutput == device.id,
                    onClick = {
                        selectedOutput = device.id
                        AudioController.selectOutput(device.id)
                    }
                )
            }
        }

        PanelCard(
            title = "Status",
            modifier = Modifier.padding(top = 12.dp)
        ) {
            Text(
                text = if (manager.hasUsbDevice())
                    "Audio interface USB terdeteksi (mis. iRig)."
                else
                    "Tidak ada perangkat USB. Gunakan mikrofon internal atau colok iRig.",
                color = TextSecondary,
                fontSize = 13.sp
            )
            Text(
                text = "Pilihan berlaku saat audio di-start ulang.",
                color = TextSecondary,
                fontSize = 12.sp,
                modifier = Modifier.padding(top = 4.dp)
            )
        }

        PanelCard(
            title = "Mesin Native",
            modifier = Modifier.padding(top = 12.dp)
        ) {
            Text(
                text = if (NativeBridge.isLoaded)
                    "Library native: OK\n${NativeBridge.version()}"
                else
                    "Library native gagal dimuat.",
                color = TextSecondary,
                fontSize = 13.sp
            )
        }

        PanelCard(
            title = "Tentang",
            modifier = Modifier.padding(top = 12.dp)
        ) {
            Text(
                text = "ALTO GUITAR FX • ALTOMEDIA\nVersi 1.0 (rancangan)",
                color = TextSecondary,
                fontSize = 13.sp
            )
        }
    }
}

@Composable
private fun DeviceRow(
    label: String,
    detail: String,
    selected: Boolean,
    onClick: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(top = 6.dp)
            .background(
                color = if (selected) MaterialTheme.colorScheme.surfaceVariant
                else MaterialTheme.colorScheme.surface,
                shape = RoundedCornerShape(10.dp)
            )
            .clickable(onClick = onClick)
            .padding(horizontal = 8.dp, vertical = 6.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        RadioButton(selected = selected, onClick = onClick)
        Column(modifier = Modifier.padding(start = 4.dp)) {
            Text(
                text = label,
                color = if (selected) Accent else MaterialTheme.colorScheme.onSurface,
                fontSize = 14.sp
            )
            if (detail.isNotEmpty()) {
                Text(text = detail, color = TextSecondary, fontSize = 11.sp)
            }
        }
    }
}

private fun deviceDetail(sampleRates: List<Int>, channels: List<Int>): String {
    val parts = mutableListOf<String>()
    if (sampleRates.isNotEmpty()) {
        val rates = sampleRates.sorted().joinToString(", ") { "${it / 1000} kHz" }
        parts += rates
    }
    if (channels.isNotEmpty()) {
        parts += "${channels.maxOrNull()} ch"
    }
    return parts.joinToString(" • ")
}
