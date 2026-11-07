package com.altomedia.altoguitarfx.ui.home

import android.Manifest
import android.content.pm.PackageManager
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.RadioButton
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.key
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.ContextCompat
import com.altomedia.altoguitarfx.audio.AudioController
import com.altomedia.altoguitarfx.audio.AudioDeviceManager
import com.altomedia.altoguitarfx.audio.NativeBridge
import com.altomedia.altoguitarfx.audio.UsbAudioPermission
import com.altomedia.altoguitarfx.data.ActivePresetStore
import com.altomedia.altoguitarfx.data.PresetRepository
import com.altomedia.altoguitarfx.effects.EffectController
import com.altomedia.altoguitarfx.effects.EffectType
import com.altomedia.altoguitarfx.navigation.Destination
import com.altomedia.altoguitarfx.ui.components.LevelMeter
import com.altomedia.altoguitarfx.ui.components.PanelCard
import com.altomedia.altoguitarfx.ui.components.PedalBox
import com.altomedia.altoguitarfx.ui.components.ScreenScaffold
import com.altomedia.altoguitarfx.ui.effects.applyQuickPreset
import com.altomedia.altoguitarfx.ui.theme.Accent
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
    var muted by remember { mutableStateOf(AudioController.isMuted()) }
    var bypassed by remember { mutableStateOf(AudioController.isGlobalBypass()) }

    // Deteksi "tidak ada input" dan rekomendasi headphone (blueprint bagian 6).
    val deviceManager = remember { AudioDeviceManager(context) }
    var headphoneConnected by remember { mutableStateOf(deviceManager.hasHeadphone()) }
    var silentFrames by remember { mutableStateOf(0) }

    DisposableEffect(Unit) {
        val callback = deviceManager.observe {
            headphoneConnected = deviceManager.hasHeadphone()
        }
        onDispose { deviceManager.stopObserving(callback) }
    }

    // Polling level dari engine native selama audio berjalan.
    LaunchedEffect(status.running) {
        while (status.running) {
            meter = AudioController.meter()
            // Level < -60 dBFS dianggap sunyi. ~1 detik (20 x 50 ms) sunyi
            // berurutan => tampilkan peringatan tidak ada input.
            silentFrames = if (meter.inputLevel < 0.001f) silentFrames + 1 else 0
            delay(50)
        }
    }

    val permissionLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.RequestPermission()
    ) { granted ->
        status = if (granted) {
            AudioController.start()
        } else {
            status.copy(message = "Izin mikrofon ditolak. Aktifkan lewat Settings.")
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
                if (silentFrames >= 20) {
                    Text(
                        text = "Tidak ada sinyal input. Periksa kabel gitar/iRig " +
                            "dan pastikan mikrofon tidak tertutup.",
                        color = Danger,
                        fontSize = 12.sp,
                        modifier = Modifier.padding(top = 8.dp)
                    )
                }
                if (!headphoneConnected) {
                    Text(
                        text = "Disarankan memakai headphone untuk mencegah feedback, " +
                            "terutama saat memakai mikrofon internal.",
                        color = TextSecondary,
                        fontSize = 12.sp,
                        modifier = Modifier.padding(top = 6.dp)
                    )
                }
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

        PedalboardPanel(onNavigate = onNavigate)

        RoutingPanel(status = status, onStatus = { status = it })

        PresetPanel(onNavigate = onNavigate)

        PanelCard(
            title = "Kontrol Utama",
            modifier = Modifier.padding(top = 12.dp)
        ) {
            Text(
                text = "Mute dan bypass berlaku pada seluruh rantai efek.",
                color = TextSecondary,
                fontSize = 12.sp
            )
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(top = 8.dp),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                OutlinedButton(
                    onClick = {
                        muted = !muted
                        AudioController.setMuted(muted)
                    },
                    colors = if (muted) {
                        ButtonDefaults.outlinedButtonColors(
                            contentColor = MaterialTheme.colorScheme.error
                        )
                    } else {
                        ButtonDefaults.outlinedButtonColors()
                    }
                ) {
                    Text(if (muted) "Mute: ON" else "Mute: OFF")
                }
                OutlinedButton(
                    onClick = {
                        bypassed = !bypassed
                        AudioController.setGlobalBypass(bypassed)
                    }
                ) {
                    Text(if (bypassed) "Bypass: ON" else "Bypass: OFF")
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

/**
 * Panel preset aktif (blueprint bagian 6: Home menampilkan preset aktif serta
 * tombol simpan/muat preset). Menyimpan keadaan DSP saat ini menjadi preset
 * pengguna baru, atau membuka layar Presets untuk memuat preset lain.
 */
@Composable
private fun PresetPanel(onNavigate: (Destination) -> Unit) {
    val context = LocalContext.current
    val repository = remember { PresetRepository(context) }
    val activeName = ActivePresetStore.name
    var showSave by remember { mutableStateOf(false) }
    var saveName by remember { mutableStateOf("") }

    PanelCard(
        title = "Preset Aktif",
        modifier = Modifier.padding(top = 12.dp)
    ) {
        Text(
            text = activeName?.let { "Preset aktif: $it" } ?: "Belum ada preset dimuat.",
            color = if (activeName != null) Accent else TextSecondary,
            fontSize = 14.sp
        )
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(top = 8.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Button(onClick = { saveName = activeName.orEmpty(); showSave = true }) {
                Text("Simpan", color = MaterialTheme.colorScheme.onPrimary)
            }
            OutlinedButton(onClick = { onNavigate(Destination.PRESETS) }) {
                Text("Muat")
            }
        }
    }

    if (showSave) {
        AlertDialog(
            onDismissRequest = { showSave = false },
            title = { Text("Simpan Preset") },
            text = {
                OutlinedTextField(
                    value = saveName,
                    onValueChange = { saveName = it },
                    singleLine = true,
                    label = { Text("Nama preset") }
                )
            },
            confirmButton = {
                TextButton(
                    enabled = saveName.isNotBlank(),
                    onClick = {
                        val preset = repository.capture(saveName.trim())
                        repository.save(preset)
                        ActivePresetStore.set(preset)
                        showSave = false
                    }
                ) { Text("Simpan") }
            },
            dismissButton = {
                TextButton(onClick = { showSave = false }) { Text("Batal") }
            }
        )
    }
}

/**
 * Pemilih input/output cepat (blueprint bagian 6: Home menampilkan pilihan
 * input Internal Mic / USB Interface dan pilihan output yang tersedia).
 * Mengganti pilihan langsung menerapkan: bila audio berjalan, stream di-start
 * ulang dengan perangkat baru.
 */
@Composable
private fun RoutingPanel(
    status: AudioController.Status,
    onStatus: (AudioController.Status) -> Unit
) {
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

    PanelCard(
        title = "Input & Output",
        modifier = Modifier.padding(top = 12.dp)
    ) {
        Text(text = "Input", color = TextSecondary, fontSize = 12.sp)
        if (inputs.isEmpty()) {
            Text(
                text = "Tidak ada perangkat input terdeteksi.",
                color = Danger,
                fontSize = 12.sp
            )
        }
        inputs.forEach { device ->
            QuickDeviceRow(
                label = device.name.ifEmpty { device.typeLabel },
                detail = device.typeLabel,
                selected = selectedInput == device.id,
                onClick = {
                    selectedInput = device.id
                    onStatus(AudioController.switchInput(device.id))
                }
            )
        }

        Text(
            text = "Output",
            color = TextSecondary,
            fontSize = 12.sp,
            modifier = Modifier.padding(top = 8.dp)
        )
        if (outputs.isEmpty()) {
            Text(
                text = "Tidak ada perangkat output terdeteksi.",
                color = Danger,
                fontSize = 12.sp
            )
        }
        outputs.forEach { device ->
            QuickDeviceRow(
                label = device.name.ifEmpty { device.typeLabel },
                detail = device.typeLabel,
                selected = selectedOutput == device.id,
                onClick = {
                    selectedOutput = device.id
                    onStatus(AudioController.switchOutput(device.id))
                }
            )
        }

        if (status.running) {
            Text(
                text = "Perangkat berganti saat audio berjalan akan di-start ulang.",
                color = TextSecondary,
                fontSize = 11.sp,
                modifier = Modifier.padding(top = 6.dp)
            )
        }

        // USB Audio Interface (iRig): status izin USB nyata dari sistem.
        val usbDevices = remember(inputs, outputs) {
            UsbAudioPermission.audioDevices(context)
        }
        if (usbDevices.isNotEmpty()) {
            Text(
                text = "USB Audio Interface",
                color = TextSecondary,
                fontSize = 12.sp,
                modifier = Modifier.padding(top = 10.dp)
            )
            usbDevices.forEach { device ->
                var granted by remember(device.deviceName) {
                    mutableStateOf(UsbAudioPermission.hasPermission(context, device))
                }
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(top = 4.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Column(modifier = Modifier.weight(1f)) {
                        Text(
                            text = device.productName ?: device.deviceName,
                            color = if (granted) Accent else MaterialTheme.colorScheme.onSurface,
                            fontSize = 13.sp
                        )
                        Text(
                            text = if (granted) "Izin USB diberikan" else "Izin USB diperlukan",
                            color = if (granted) TextSecondary else Danger,
                            fontSize = 11.sp
                        )
                    }
                    if (!granted) {
                        OutlinedButton(
                            onClick = {
                                UsbAudioPermission.request(context, device) { ok -> granted = ok }
                            }
                        ) {
                            Text("Izinkan")
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun QuickDeviceRow(
    label: String,
    detail: String,
    selected: Boolean,
    onClick: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(top = 4.dp)
            .clickable(onClick = onClick),
        verticalAlignment = Alignment.CenterVertically
    ) {
        RadioButton(selected = selected, onClick = onClick)
        Column(modifier = Modifier.padding(start = 4.dp)) {
            Text(
                text = label,
                color = if (selected) Accent else MaterialTheme.colorScheme.onSurface,
                fontSize = 13.sp
            )
            Text(text = detail, color = TextSecondary, fontSize = 11.sp)
        }
    }
}

/**
 * Strip pedalboard di Home (blueprint bagian 6: 'Pedalboard dengan status
 * ON/OFF dan kontrol parameter'). Menampilkan efek sebagai kotak pedal dengan
 * tombol ON/OFF bergaya sakelar kaki, tombol Preset cepat, dan tombol Atur
 * untuk membuka layar pengaturan efek tersebut.
 */
@Composable
private fun PedalboardPanel(onNavigate: (Destination) -> Unit) {
    // Urutan diambil dari mesin native agar cocok dengan yang diproses DSP.
    fun readPedals(): List<Pair<EffectType, Boolean>> {
        val count = NativeBridge.effectCount()
        return (0 until count).mapNotNull { slot ->
            val id = NativeBridge.effectOrderAt(slot)
            val type = EffectType.values().firstOrNull { it.id == id } ?: return@mapNotNull null
            type to EffectController.isEnabled(type)
        }
    }

    var pedals by remember { mutableStateOf(readPedals()) }
    var presetIndex by remember { mutableStateOf(emptyMap<EffectType, Int>()) }

    PanelCard(
        title = "Pedalboard",
        modifier = Modifier.padding(top = 12.dp)
    ) {
        Text(
            text = "Aktifkan/nonaktifkan efek atau pilih preset cepat. Atur " +
                "parameter di layar Effects.",
            color = TextSecondary,
            fontSize = 12.sp
        )
        pedals.chunked(2).forEach { rowItems ->
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(top = 8.dp),
                horizontalArrangement = Arrangement.spacedBy(10.dp)
            ) {
                rowItems.forEach { (type, enabled) ->
                    key(type) {
                        val presetName = type.quickPresets
                            .getOrNull((presetIndex[type] ?: 0) % maxOf(1, type.quickPresets.size))
                            ?.name
                        PedalBox(
                            title = type.label,
                            enabled = enabled,
                            presetLabel = presetName,
                            onToggle = {
                                EffectController.setEnabled(type, it)
                                pedals = readPedals()
                            },
                            onPreset = if (type.quickPresets.isEmpty()) null else {
                                {
                                    applyQuickPreset(type, presetIndex[type] ?: 0)
                                    presetIndex = presetIndex.toMutableMap().also {
                                        it[type] = ((presetIndex[type] ?: 0) + 1) %
                                            type.quickPresets.size
                                    }
                                    pedals = readPedals()
                                }
                            },
                            onOpen = { onNavigate(Destination.EFFECTS) },
                            modifier = Modifier.weight(1f)
                        )
                    }
                }
                if (rowItems.size == 1) {
                    Column(modifier = Modifier.weight(1f)) {}
                }
            }
        }
        OutlinedButton(
            onClick = { onNavigate(Destination.EFFECTS) },
            modifier = Modifier.padding(top = 10.dp)
        ) {
            Text("Buka Pedalboard")
        }
    }
}
