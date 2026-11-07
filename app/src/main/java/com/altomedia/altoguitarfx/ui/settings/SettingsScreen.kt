package com.altomedia.altoguitarfx.ui.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.RadioButton
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
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
import com.altomedia.altoguitarfx.ui.theme.Danger
import com.altomedia.altoguitarfx.ui.theme.TextSecondary
import kotlinx.coroutines.delay

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
    var micGranted by remember {
        mutableStateOf(
            androidx.core.content.ContextCompat.checkSelfPermission(
                context, android.Manifest.permission.RECORD_AUDIO
            ) == android.content.pm.PackageManager.PERMISSION_GRANTED
        )
    }
    val permissionLauncher = androidx.activity.compose.rememberLauncherForActivityResult(
        androidx.activity.result.contract.ActivityResultContracts.RequestPermission()
    ) { granted -> micGranted = granted }

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
        PanelCard(title = "Izin") {
            Text(
                text = "Izin mikrofon: " + if (micGranted) "diberikan" else "belum diberikan",
                color = if (micGranted) Accent else Danger,
                fontSize = 13.sp
            )
            Text(
                text = "Diperlukan untuk menangkap sinyal gitar dari mikrofon " +
                    "internal maupun audio interface USB (iRig).",
                color = TextSecondary,
                fontSize = 12.sp,
                modifier = Modifier.padding(top = 4.dp)
            )
            if (!micGranted) {
                Row(
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                    modifier = Modifier.padding(top = 8.dp)
                ) {
                    OutlinedButton(
                        onClick = {
                            permissionLauncher.launch(android.Manifest.permission.RECORD_AUDIO)
                        }
                    ) {
                        Text("Minta Izin Mikrofon")
                    }
                    // Bila izin ditolak permanen, dialog tidak muncul lagi;
                    // arahkan pengguna ke pengaturan aplikasi (blueprint bagian 6).
                    OutlinedButton(
                        onClick = {
                            context.startActivity(
                                android.content.Intent(
                                    android.provider.Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                                    android.net.Uri.fromParts(
                                        "package", context.packageName, null
                                    )
                                )
                            )
                        }
                    ) {
                        Text("Buka Pengaturan Aplikasi")
                    }
                }
            }
        }

        PanelCard(
            title = "Sumber Input",
            modifier = Modifier.padding(top = 12.dp)
        ) {
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
            title = "Sample Rate",
            modifier = Modifier.padding(top = 12.dp)
        ) {
            // Rate yang didukung diambil dari perangkat output terpilih.
            val available = remember(outputs, selectedOutput) {
                val device = outputs.firstOrNull { it.id == selectedOutput }
                val fromDevice = device?.sampleRates
                    ?.filter { it in 8000..192000 }
                    ?.distinct()
                    ?.sorted()
                    .orEmpty()
                if (fromDevice.isNotEmpty()) fromDevice else listOf(44100, 48000)
            }
            var chosen by remember { mutableStateOf(AudioController.selectedSampleRate()) }
            var activeRate by remember { mutableStateOf(AudioController.sampleRate()) }
            LaunchedEffect(Unit) {
                while (true) {
                    activeRate = AudioController.sampleRate()
                    delay(500)
                }
            }
            Text(
                text = "Rate stream aktif: " +
                    if (activeRate > 0) "$activeRate Hz" else "belum berjalan",
                color = if (activeRate > 0) Accent else TextSecondary,
                fontSize = 13.sp
            )
            Text(
                text = "Rate yang didukung perangkat output terpilih. Bila rate " +
                    "pilihan tidak didukung, stream memakai rate bawaan perangkat.",
                color = TextSecondary,
                fontSize = 12.sp,
                modifier = Modifier.padding(top = 4.dp)
            )
            available.forEach { rate ->
                DeviceRow(
                    label = "$rate Hz",
                    detail = if (rate == activeRate) "sedang dipakai" else "",
                    selected = chosen == rate,
                    onClick = {
                        chosen = rate
                        AudioController.setPreferredSampleRate(rate)
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
                    "Tidak ada perangkat USB. iRig yang dicolok ke lubang " +
                        "headset akan muncul sebagai 'Headset kabel' di Sumber " +
                        "Input; pilih perangkat itu bila belum terpilih otomatis.",
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

        LevelPanel()

        PerformancePanel()

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
            // Versi sebenarnya dibaca dari PackageManager (bukan hardcode).
            val packageInfo = remember {
                runCatching {
                    context.packageManager.getPackageInfo(context.packageName, 0)
                }.getOrNull()
            }
            val versionName = packageInfo?.versionName ?: "-"
            val versionCode = if (android.os.Build.VERSION.SDK_INT >= 28) {
                packageInfo?.longVersionCode?.toString() ?: "-"
            } else {
                @Suppress("DEPRECATION")
                packageInfo?.versionCode?.toString() ?: "-"
            }
            Text(
                text = "ALTO GUITAR FX • ALTOMEDIA",
                color = MaterialTheme.colorScheme.onSurface,
                fontSize = 14.sp
            )
            Text(
                text = "Versi $versionName ($versionCode) • ${context.packageName}",
                color = TextSecondary,
                fontSize = 12.sp,
                modifier = Modifier.padding(top = 2.dp)
            )
            Text(
                text = "Real-time guitar multi-effects (NDK + Compose).",
                color = TextSecondary,
                fontSize = 12.sp,
                modifier = Modifier.padding(top = 6.dp)
            )
        }

        PanelCard(
            title = "Kompatibilitas & Rekomendasi",
            modifier = Modifier.padding(top = 12.dp)
        ) {
            Text(
                text = "• Input: mikrofon internal atau USB Audio Interface " +
                    "(iRig). Satu input aktif pada versi ini.",
                color = TextSecondary,
                fontSize = 12.sp
            )
            Text(
                text = "• Gunakan headphone untuk mencegah feedback, terutama " +
                    "saat memakai mikrofon internal.",
                color = TextSecondary,
                fontSize = 12.sp,
                modifier = Modifier.padding(top = 4.dp)
            )
            Text(
                text = "• Bila interface USB tidak terdeteksi, cabut dan pasang " +
                    "ulang, lalu pilih kembali di panel Sumber Input.",
                color = TextSecondary,
                fontSize = 12.sp,
                modifier = Modifier.padding(top = 4.dp)
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

/**
 * Panel performa & latensi (BAB 16): beban CPU callback, waktu eksekusi,
 * kapasitas buffer, dan estimasi latensi round-trip dari buffer nyata.
 */
@Composable
private fun PerformancePanel() {
    var perf by remember { mutableStateOf(AudioController.performance()) }

    LaunchedEffect(Unit) {
        while (true) {
            perf = AudioController.performance()
            delay(200)
        }
    }

    val loadPercent = (perf.averageCpuLoad * 100f)
    val peakPercent = (perf.peakCpuLoad * 100f)
    val overload = perf.peakCpuLoad > 0.8f

    PanelCard(
        title = "Performa & Latensi",
        modifier = Modifier.padding(top = 12.dp)
    ) {
        Text(
            text = "Beban callback diukur terhadap anggaran waktu satu blok. " +
                "Nilai di bawah 80% umumnya aman dari drop-out.",
            color = TextSecondary,
            fontSize = 12.sp
        )
        Text(
            text = "Beban rata-rata: ${"%.1f".format(loadPercent)}%\n" +
                "Beban puncak: ${"%.1f".format(peakPercent)}%",
            color = if (overload) Danger else TextSecondary,
            fontSize = 12.sp,
            modifier = Modifier.padding(top = 6.dp)
        )
        Text(
            text = "Callback rata-rata: ${"%.0f".format(perf.averageCallbackMicros)} us\n" +
                "Callback puncak: ${"%.0f".format(perf.peakCallbackMicros)} us",
            color = TextSecondary,
            fontSize = 12.sp,
            modifier = Modifier.padding(top = 4.dp)
        )
        Text(
            text = "Buffer: ${perf.bufferCapacityFrames} frame\n" +
                "Xrun (drop-out): ${perf.xrunCount}",
            color = if (perf.xrunCount > 0) Danger else TextSecondary,
            fontSize = 12.sp,
            modifier = Modifier.padding(top = 4.dp)
        )
        val measured = perf.measuredLatencyMs
        Text(
            text = if (measured >= 0f)
                "Latensi terukur (timestamp perangkat): " +
                    "${"%.1f".format(measured)} ms"
            else
                "Latensi terukur: perangkat ini tidak melaporkan timestamp " +
                    "yang dapat dipercaya.",
            color = if (measured >= 0f) Accent else TextSecondary,
            fontSize = 12.sp,
            modifier = Modifier.padding(top = 4.dp)
        )
        Text(
            text = "Estimasi dari ukuran buffer: " +
                "${"%.1f".format(perf.estimatedLatencyMs)} ms (perkiraan, bukan " +
                "hasil pengukuran).",
            color = TextSecondary,
            fontSize = 12.sp,
            modifier = Modifier.padding(top = 2.dp)
        )
        Text(
            text = "Latensi terukur dihitung dari timestamp Oboe saat audio " +
                "berjalan; pastikan stream aktif agar nilainya muncul.",
            color = TextSecondary,
            fontSize = 11.sp,
            modifier = Modifier.padding(top = 2.dp)
        )
        OutlinedButton(
            onClick = { AudioController.resetPerformance() },
            modifier = Modifier.padding(top = 8.dp)
        ) {
            Text("Reset Statistik")
        }
    }
}

/**
 * Panel level & pengaturan audio (BAB 15): trim input, level output master,
 * batas limiter, indikator clipping, dan pesan kesalahan audio terakhir.
 */
@Composable
private fun LevelPanel() {
    var inputGain by remember { mutableStateOf(AudioController.inputGainDb()) }
    var outputGain by remember { mutableStateOf(AudioController.outputGainDb()) }
    var ceiling by remember { mutableStateOf(AudioController.limiterCeilingDb()) }
    var humHz by remember { mutableStateOf(AudioController.humFilterHz()) }
    var levels by remember { mutableStateOf(AudioController.levels()) }

    LaunchedEffect(Unit) {
        while (true) {
            levels = AudioController.levels()
            delay(100)
        }
    }

    PanelCard(
        title = "Level & Pengaturan Audio",
        modifier = Modifier.padding(top = 12.dp)
    ) {
        Text(
            text = "Trim input dan level output diterapkan dengan perataan " +
                "per-sampel sehingga tidak menimbulkan klik.",
            color = TextSecondary,
            fontSize = 12.sp
        )

        GainSlider(
            label = "Trim Input",
            value = inputGain,
            range = -24f..24f,
            onChange = {
                inputGain = it
                AudioController.setInputGainDb(it)
            }
        )
        GainSlider(
            label = "Level Output",
            value = outputGain,
            range = -40f..18f,
            onChange = {
                outputGain = it
                AudioController.setOutputGainDb(it)
            }
        )
        GainSlider(
            label = "Batas Limiter",
            value = ceiling,
            range = -12f..0f,
            onChange = {
                ceiling = it
                AudioController.setLimiterCeilingDb(it)
            }
        )

        // Peredam dengung/grounding (notch 50/60 Hz + harmonisa).
        Text(
            text = "Peredam Dengung (Grounding)",
            color = MaterialTheme.colorScheme.onSurface,
            fontSize = 13.sp,
            modifier = Modifier.padding(top = 12.dp)
        )
        Text(
            text = "Menekan dengung 50/60 Hz dan harmonisanya. Pilih sesuai " +
                "frekuensi listrik di tempat Anda (Indonesia 50 Hz).",
            color = TextSecondary,
            fontSize = 12.sp,
            modifier = Modifier.padding(top = 2.dp)
        )
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(top = 6.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            HumOption(
                label = "Off",
                selected = humHz <= 0f,
                modifier = Modifier.weight(1f),
                onClick = {
                    humHz = 0f
                    AudioController.setHumFilterHz(0f)
                }
            )
            HumOption(
                label = "50 Hz",
                selected = humHz > 0f && humHz < 55f,
                modifier = Modifier.weight(1f),
                onClick = {
                    humHz = 50f
                    AudioController.setHumFilterHz(50f)
                }
            )
            HumOption(
                label = "60 Hz",
                selected = humHz >= 55f,
                modifier = Modifier.weight(1f),
                onClick = {
                    humHz = 60f
                    AudioController.setHumFilterHz(60f)
                }
            )
        }

        Text(
            text = "Bila keluaran terasa terlalu kecil, naikkan Level Output " +
                "atau Trim Input. Limiter tetap menjaga puncak pada batas agar " +
                "tidak terjadi clipping keras.",
            color = TextSecondary,
            fontSize = 11.sp,
            modifier = Modifier.padding(top = 6.dp)
        )

        Text(
            text = buildString {
                append("Clipping input: ")
                append(if (levels.inputClipping) "YA" else "tidak")
                append("\nClipping output: ")
                append(if (levels.outputClipping) "YA" else "tidak")
            },
            color = if (levels.inputClipping || levels.outputClipping) Danger else TextSecondary,
            fontSize = 12.sp,
            modifier = Modifier.padding(top = 6.dp)
        )

        if (levels.lastError.isNotEmpty()) {
            Text(
                text = levels.lastError,
                color = Danger,
                fontSize = 12.sp,
                modifier = Modifier.padding(top = 6.dp)
            )
        }
    }
}

@Composable
private fun GainSlider(
    label: String,
    value: Float,
    range: ClosedFloatingPointRange<Float>,
    onChange: (Float) -> Unit
) {
    Column(modifier = Modifier.padding(top = 8.dp)) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(text = label, color = TextSecondary, fontSize = 12.sp)
            Text(
                text = "${"%.1f".format(value)} dB",
                color = MaterialTheme.colorScheme.onSurface,
                fontSize = 12.sp
            )
        }
        Slider(
            value = value.coerceIn(range),
            onValueChange = onChange,
            valueRange = range
        )
    }
}

/** Tombol pilihan frekuensi dengung (Off / 50 Hz / 60 Hz). */
@Composable
private fun HumOption(
    label: String,
    selected: Boolean,
    modifier: Modifier = Modifier,
    onClick: () -> Unit
) {
    OutlinedButton(
        onClick = onClick,
        modifier = modifier,
        colors = if (selected) {
            androidx.compose.material3.ButtonDefaults.outlinedButtonColors(contentColor = Accent)
        } else {
            androidx.compose.material3.ButtonDefaults.outlinedButtonColors()
        }
    ) {
        Text(label, fontSize = 13.sp)
    }
}
