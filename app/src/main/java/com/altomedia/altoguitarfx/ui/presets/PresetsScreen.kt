package com.altomedia.altoguitarfx.ui.presets

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.altomedia.altoguitarfx.data.ActivePresetStore
import com.altomedia.altoguitarfx.data.BuiltInPresets
import com.altomedia.altoguitarfx.data.Preset
import com.altomedia.altoguitarfx.data.PresetRepository
import com.altomedia.altoguitarfx.data.ToneLibrary
import com.altomedia.altoguitarfx.ui.components.PanelCard
import com.altomedia.altoguitarfx.ui.components.ScreenScaffold
import com.altomedia.altoguitarfx.ui.theme.Accent
import com.altomedia.altoguitarfx.ui.theme.PanelBgAlt
import com.altomedia.altoguitarfx.ui.theme.TextSecondary

/**
 * Layar Tone bergaya Tonebridge: daftar tone siap pakai yang dinamai sesuai
 * lagu/artis dengan pencarian, plus tab Preset Bawaan dan Preset Saya untuk
 * menyimpan/mengelola preset pengguna.
 */
@Composable
fun PresetsScreen() {
    val context = LocalContext.current
    val repository = remember { PresetRepository(context) }
    var tab by remember { mutableStateOf(ToneTab.TONES) }
    var query by remember { mutableStateOf("") }
    var message by remember { mutableStateOf("") }
    var dialog by remember { mutableStateOf<PresetDialog?>(null) }
    var confirmRestore by remember { mutableStateOf(false) }

    var userPresets by remember { mutableStateOf(repository.load().filter { !it.builtIn }) }
    val activeName = ActivePresetStore.name

    fun load(preset: Preset) {
        repository.apply(preset)
        ActivePresetStore.set(preset)
        message = "Tone '${preset.name}' dimuat."
    }

    ScreenScaffold(
        title = "Tone",
        subtitle = "Tone siap pakai, preset bawaan, dan preset saya"
    ) {
        // ScreenScaffold sudah dapat di-scroll; jangan menambahkan scroll
        // bersarang (dapat memicu kesalahan pengukuran tinggi tak terhingga).
        // Tab bergaya Tonebridge.
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .clip(RoundedCornerShape(12.dp))
                .background(PanelBgAlt.copy(alpha = 0.5f))
                .padding(4.dp),
            horizontalArrangement = Arrangement.spacedBy(4.dp)
        ) {
            ToneTab.entries.forEach { entry ->
                val selected = tab == entry
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .clip(RoundedCornerShape(9.dp))
                        .background(if (selected) Accent else PanelBgAlt.copy(alpha = 0.45f))
                        .clickable { tab = entry }
                        .padding(vertical = 8.dp),
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        text = entry.label,
                        color = if (selected) androidx.compose.ui.graphics.Color.Black
                        else MaterialTheme.colorScheme.onSurface,
                        fontSize = 13.sp,
                        fontWeight = FontWeight.SemiBold
                    )
                }
            }
        }

        if (message.isNotEmpty()) {
            Text(
                text = message,
                color = Accent,
                fontSize = 12.sp,
                modifier = Modifier.padding(top = 8.dp)
            )
        }

        when (tab) {
            ToneTab.TONES -> ToneList(
                query = query,
                onQueryChange = { query = it },
                activeName = activeName,
                onLoad = { tone -> load(ToneLibrary.preset(tone)) }
            )
            ToneTab.BUILTIN -> BuiltinList(activeName = activeName, onLoad = ::load)
            ToneTab.MINE -> MyPresets(
                presets = userPresets,
                activeName = activeName,
                onLoad = ::load,
                onSaveCurrent = { dialog = PresetDialog.SaveNew },
                onRename = { dialog = PresetDialog.Rename(it) },
                onDuplicate = { dialog = PresetDialog.Duplicate(it) },
                onDelete = {
                    repository.delete(it.name)
                    if (activeName == it.name) ActivePresetStore.setName(null, false)
                    userPresets = repository.load().filter { p -> !p.builtIn }
                    message = "Preset '${it.name}' dihapus."
                },
                onRestoreDefault = { confirmRestore = true }
            )
        }
    }

    if (confirmRestore) {
        AlertDialog(
            onDismissRequest = { confirmRestore = false },
            title = { Text("Restore Default") },
            text = { Text("Hapus semua preset pengguna dan kembalikan ke CLEAN?") },
            confirmButton = {
                TextButton(onClick = {
                    repository.clearUserPresets()
                    val clean = BuiltInPresets.all().first()
                    repository.apply(clean)
                    ActivePresetStore.set(clean)
                    userPresets = repository.load().filter { !it.builtIn }
                    message = "Preset dikembalikan ke bawaan."
                    confirmRestore = false
                }) { Text("Ya, Restore") }
            },
            dismissButton = {
                TextButton(onClick = { confirmRestore = false }) { Text("Batal") }
            }
        )
    }

    dialog?.let { request ->
        NameDialog(
            request = request,
            onConfirm = { name ->
                when (request) {
                    PresetDialog.SaveNew -> {
                        repository.save(repository.capture(name))
                        ActivePresetStore.setName(name, true)
                        message = "Preset '$name' disimpan."
                    }
                    is PresetDialog.Rename -> {
                        repository.rename(request.preset.name, name)
                        if (activeName == request.preset.name) {
                            ActivePresetStore.setName(name, true)
                        }
                        message = "Preset diganti nama menjadi '$name'."
                    }
                    is PresetDialog.Duplicate -> {
                        repository.duplicate(request.preset, name)
                        message = "Preset diduplikasi menjadi '$name'."
                    }
                }
                userPresets = repository.load().filter { !it.builtIn }
                dialog = null
            },
            onDismiss = { dialog = null }
        )
    }
}

/** Tab pada layar Tone. */
private enum class ToneTab(val label: String) {
    TONES("Tone"),
    BUILTIN("Bawaan"),
    MINE("Preset Saya")
}

@Composable
private fun ToneList(
    query: String,
    onQueryChange: (String) -> Unit,
    activeName: String?,
    onLoad: (ToneLibrary.Tone) -> Unit
) {
    OutlinedTextField(
        value = query,
        onValueChange = onQueryChange,
        singleLine = true,
        label = { Text("Cari lagu atau artis") },
        modifier = Modifier
            .fillMaxWidth()
            .padding(top = 10.dp)
    )

    val tones = remember(query) {
        val q = query.trim().lowercase()
        ToneLibrary.all().filter {
            q.isEmpty() || it.name.lowercase().contains(q) ||
                it.artist.lowercase().contains(q) || it.style.lowercase().contains(q)
        }
    }

    if (tones.isEmpty()) {
        Text(
            text = "Tidak ada tone yang cocok dengan pencarian.",
            color = TextSecondary,
            fontSize = 13.sp,
            modifier = Modifier.padding(top = 12.dp)
        )
    }

    tones.forEach { tone ->
        ToneCard(tone = tone, active = tone.name == activeName, onLoad = { onLoad(tone) })
    }
}

@Composable
private fun ToneCard(tone: ToneLibrary.Tone, active: Boolean, onLoad: () -> Unit) {
    PanelCard(
        title = tone.name,
        modifier = Modifier.padding(top = 10.dp)
    ) {
        Text(
            text = "${tone.artist} • ${tone.style}",
            color = TextSecondary,
            fontSize = 12.sp
        )
        Text(
            text = "★".repeat(tone.rating) + "☆".repeat(5 - tone.rating),
            color = Accent,
            fontSize = 13.sp,
            modifier = Modifier.padding(top = 2.dp)
        )
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(top = 8.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Button(onClick = onLoad) {
                Text(if (active) "Muat Ulang" else "Muat", color = MaterialTheme.colorScheme.onPrimary)
            }
            if (active) {
                Text("AKTIF", color = Accent, fontSize = 12.sp, fontWeight = FontWeight.Bold)
            }
        }
    }
}

@Composable
private fun BuiltinList(activeName: String?, onLoad: (Preset) -> Unit) {
    BuiltInPresets.all().forEach { preset ->
        PresetRow(
            preset = preset,
            active = preset.name == activeName,
            onLoad = { onLoad(preset) },
            onRename = null,
            onDuplicate = null,
            onDelete = null
        )
    }
}

@Composable
private fun MyPresets(
    presets: List<Preset>,
    activeName: String?,
    onLoad: (Preset) -> Unit,
    onSaveCurrent: () -> Unit,
    onRename: (Preset) -> Unit,
    onDuplicate: (Preset) -> Unit,
    onDelete: (Preset) -> Unit,
    onRestoreDefault: () -> Unit
) {
    PanelCard(title = "Simpan Keadaan Saat Ini") {
        Text(
            text = "Menyimpan urutan efek, status aktif, dan seluruh nilai " +
                "parameter yang sedang aktif menjadi preset baru.",
            color = TextSecondary,
            fontSize = 12.sp
        )
        Button(
            onClick = onSaveCurrent,
            modifier = Modifier.padding(top = 8.dp)
        ) {
            Text("Simpan Baru", color = MaterialTheme.colorScheme.onPrimary)
        }
    }

    if (presets.isEmpty()) {
        Text(
            text = "Belum ada preset pengguna. Gunakan 'Simpan Baru' untuk " +
                "menambahkan.",
            color = TextSecondary,
            fontSize = 12.sp,
            modifier = Modifier.padding(top = 10.dp)
        )
    }

    presets.forEach { preset ->
        PresetRow(
            preset = preset,
            active = preset.name == activeName,
            onLoad = { onLoad(preset) },
            onRename = { onRename(preset) },
            onDuplicate = { onDuplicate(preset) },
            onDelete = { onDelete(preset) }
        )
    }

    PanelCard(
        title = "Restore Default",
        modifier = Modifier.padding(top = 10.dp)
    ) {
        Text(
            text = "Menghapus seluruh preset pengguna dan memuat preset bawaan " +
                "CLEAN. Tindakan ini tidak dapat dibatalkan.",
            color = TextSecondary,
            fontSize = 12.sp
        )
        OutlinedButton(
            onClick = onRestoreDefault,
            modifier = Modifier.padding(top = 8.dp)
        ) {
            Text("Restore Default")
        }
    }
}

/** Permintaan dialog yang butuh nama preset. */
private sealed interface PresetDialog {
    data object SaveNew : PresetDialog
    data class Rename(val preset: Preset) : PresetDialog
    data class Duplicate(val preset: Preset) : PresetDialog
}

@Composable
private fun PresetRow(
    preset: Preset,
    active: Boolean,
    onLoad: () -> Unit,
    onRename: (() -> Unit)?,
    onDuplicate: (() -> Unit)?,
    onDelete: (() -> Unit)?
) {
    PanelCard(
        title = preset.name,
        modifier = Modifier.padding(top = 10.dp)
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(
                text = if (preset.builtIn) "Bawaan" else "Pengguna",
                color = TextSecondary,
                fontSize = 12.sp
            )
            if (active) {
                Text(
                    text = "AKTIF",
                    color = Accent,
                    fontSize = 12.sp,
                    fontWeight = FontWeight.Bold
                )
            }
        }
        Text(
            text = "Efek aktif: ${preset.enabled.size}",
            color = TextSecondary,
            fontSize = 12.sp,
            modifier = Modifier.padding(top = 2.dp)
        )
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(top = 8.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Button(onClick = onLoad) {
                Text("Muat", color = MaterialTheme.colorScheme.onPrimary)
            }
            if (onRename != null) {
                OutlinedButton(onClick = onRename) { Text("Ganti Nama") }
            }
            if (onDuplicate != null) {
                OutlinedButton(onClick = onDuplicate) { Text("Duplikat") }
            }
            if (onDelete != null) {
                OutlinedButton(
                    onClick = onDelete,
                    colors = ButtonDefaults.outlinedButtonColors(
                        contentColor = MaterialTheme.colorScheme.error
                    )
                ) { Text("Hapus") }
            }
        }
    }
}

@Composable
private fun NameDialog(
    request: PresetDialog,
    onConfirm: (String) -> Unit,
    onDismiss: () -> Unit
) {
    val initial = when (request) {
        PresetDialog.SaveNew -> ""
        is PresetDialog.Rename -> request.preset.name
        is PresetDialog.Duplicate -> "${request.preset.name} Copy"
    }
    var name by remember { mutableStateOf(initial) }
    val title = when (request) {
        PresetDialog.SaveNew -> "Simpan Preset Baru"
        is PresetDialog.Rename -> "Ganti Nama Preset"
        is PresetDialog.Duplicate -> "Duplikasi Preset"
    }

    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(title) },
        text = {
            OutlinedTextField(
                value = name,
                onValueChange = { name = it },
                label = { Text("Nama preset") },
                singleLine = true
            )
        },
        confirmButton = {
            TextButton(
                onClick = { if (name.isNotBlank()) onConfirm(name.trim()) },
                enabled = name.isNotBlank()
            ) {
                Text("Simpan")
            }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) { Text("Batal") }
        }
    )
}
