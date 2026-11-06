package com.altomedia.altoguitarfx.ui.home

import android.Manifest
import android.content.pm.PackageManager
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.ContextCompat
import com.altomedia.altoguitarfx.audio.AudioController
import com.altomedia.altoguitarfx.navigation.Destination
import com.altomedia.altoguitarfx.ui.components.LevelMeter
import com.altomedia.altoguitarfx.ui.components.PanelCard
import com.altomedia.altoguitarfx.ui.components.ScreenScaffold
import com.altomedia.altoguitarfx.ui.theme.Danger
import com.altomedia.altoguitarfx.ui.theme.TextSecondary
import kotlinx.coroutines.delay

/**
 * Home / Pedalboard. Pada BAB 4 berisi kontrol start/stop audio dan status
 * stream. Pedalboard, level meter, dan preset ditambahkan pada BAB berikutnya.
 */
@Composable
fun HomeScreen(onNavigate: (Destination) -> Unit) {
    val context = LocalContext.current
    var status by remember { mutableStateOf(AudioController.refresh()) }
    var meter by remember { mutableStateOf(AudioController.meter()) }

    // Polling level dari engine native selama audio berjalan.
    LaunchedEffect(status.running) {
        while (status.running) {
            meter = AudioController.meter()
            delay(50)
        }
    }

    val permissionLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.RequestPermission()
    ) { granted ->
        status = if (granted) {
            AudioController.start()
        } else {
            status.copy(message = "Izin mikrofon ditolak")
        }
    }

    fun startWithPermission() {
        val granted = ContextCompat.checkSelfPermission(
            context,
            Manifest.permission.RECORD_AUDIO
        ) == PackageManager.PERMISSION_GRANTED
        if (granted) {
            status = AudioController.start()
        } else {
            permissionLauncher.launch(Manifest.permission.RECORD_AUDIO)
        }
    }

    ScreenScaffold(
        title = "ALTO GUITAR FX",
        subtitle = "Pedalboard • Professional Multi Effects"
    ) {
        PanelCard(title = "Status Audio") {
            Text(
                text = if (status.running) "Audio AKTIF" else "Audio berhenti",
                color = if (status.running) MaterialTheme.colorScheme.primary else TextSecondary,
                fontSize = 15.sp
            )
            if (status.running) {
                Text(
                    text = "Sample rate: ${status.sampleRate} Hz\n" +
                        "Buffer: ${status.framesPerBurst} frame",
                    color = TextSecondary,
                    fontSize = 13.sp,
                    modifier = Modifier.padding(top = 4.dp)
                )
            }
            if (status.message.isNotEmpty()) {
                Text(
                    text = status.message,
                    color = TextSecondary,
                    fontSize = 12.sp,
                    modifier = Modifier.padding(top = 4.dp)
                )
            }

            if (status.running) {
                LevelMeter(
                    label = "Input",
                    level = meter.inputLevel,
                    modifier = Modifier.padding(top = 12.dp)
                )
                LevelMeter(
                    label = "Output",
                    level = meter.outputLevel,
                    modifier = Modifier.padding(top = 8.dp)
                )
                Text(
                    text = "Callback: ${meter.callbackCount} • xrun: ${meter.xrunCount}",
                    color = TextSecondary,
                    fontSize = 11.sp,
                    modifier = Modifier.padding(top = 8.dp)
                )
            }

            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(top = 12.dp),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Button(
                    onClick = { startWithPermission() },
                    enabled = !status.running
                ) {
                    Text("Start", color = MaterialTheme.colorScheme.onPrimary)
                }
                OutlinedButton(
                    onClick = { status = AudioController.stop() },
                    enabled = status.running
                ) {
                    Text("Stop", color = Danger)
                }
            }
        }

        PanelCard(
            title = "Pintasan",
            modifier = Modifier.padding(top = 12.dp)
        ) {
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Button(onClick = { onNavigate(Destination.EFFECTS) }) {
                    Text("Efek", color = MaterialTheme.colorScheme.onPrimary)
                }
                Button(onClick = { onNavigate(Destination.PRESETS) }) {
                    Text("Preset", color = MaterialTheme.colorScheme.onPrimary)
                }
                Button(onClick = { onNavigate(Destination.TUNER) }) {
                    Text("Tuner", color = MaterialTheme.colorScheme.onPrimary)
                }
            }
        }
    }
}
