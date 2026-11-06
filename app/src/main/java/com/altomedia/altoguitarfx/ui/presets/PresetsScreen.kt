package com.altomedia.altoguitarfx.ui.presets

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.altomedia.altoguitarfx.data.Preset
import com.altomedia.altoguitarfx.data.PresetRepository
import com.altomedia.altoguitarfx.ui.components.PanelCard
import com.altomedia.altoguitarfx.ui.components.ScreenScaffold
import com.altomedia.altoguitarfx.ui.theme.Accent
import com.altomedia.altoguitarfx.ui.theme.TextSecondary

/**
 * Presets (BAB 13). Memuat, menyimpan, mengganti nama, menduplikasi, dan
 * menghapus preset. Semua aksi benar-benar mengubah mesin DSP dan penyimpanan.
 */
@Composable
fun PresetsScreen() {
    val context = LocalContext.current
    val repository = remember { PresetRepository(context) }
    var presets by remember { mutableStateOf(repository.load()) }
    var activeName by remember { mutableStateOf<String?>(null) }
    var dialog by remember { mutableStateOf<PresetDialog?>(null) }
    var message by remember { mutableStateOf("") }

    fun refresh() {
        presets = repository.load()
    }

    ScreenScaffold(
        title = "Presets",
        subtitle = "Preset bawaan dan preset pengguna"
    ) {
        Column(modifier = Modifier.verticalScroll(rememberScrollState())) {
            PanelCard(title = "Simpan Keadaan Saat Ini") {
                Text(
                    text = "Menyimpan urutan efek, status aktif, dan seluruh " +
                        "nilai parameter yang sedang aktif menjadi preset baru.",
                    color = TextSecondary,
                    fontSize = 12.sp
                )
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(top = 8.dp),
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    Button(
                        onClick = { dialog = PresetDialog.SaveNew },
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("Simpan Baru")
                    }
                    OutlinedButton(
                        onClick = {
                            activeName?.let { name ->
                                repository.save(repository.capture(name))
                                refresh()
                                message = "Preset '$name' diperbarui."
                            }
                        },
                        enabled = activeName != null,
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("Perbarui Aktif")
                    }
                }
                if (activeName != null) {
                    Text(
                        text = "Preset aktif: $activeName",
                        color = Accent,
                        fontSize = 12.sp,
                        modifier = Modifier.padding(top = 6.dp)
                    )
                }
                if (message.isNotEmpty()) {
                    Text(
                        text = message,
                        color = TextSecondary,
                        fontSize = 12.sp,
                        modifier = Modifier.padding(top = 6.dp)
                    )
                }
            }

            presets.forEach { preset ->
                PresetRow(
                    preset = preset,
                    active = preset.name == activeName,
                    onLoad = {
                        repository.apply(preset)
                        activeName = preset.name
                        message = "Preset '${preset.name}' dimuat."
                    },
                    onRename = { dialog = PresetDialog.Rename(preset) },
                    onDuplicate = { dialog = PresetDialog.Duplicate(preset) },
                    onDelete = {
                        repository.delete(preset.name)
                        if (activeName == preset.name) activeName = null
                        refresh()
                        message = "Preset '${preset.name}' dihapus."
                    }
                )
            }

            if (presets.none { !it.builtIn }) {
                Text(
                    text = "Belum ada preset pengguna. Gunakan 'Simpan Baru' " +
                        "untuk menambahkan.",
                    color = TextSecondary,
                    fontSize = 12.sp,
                    modifier = Modifier.padding(vertical = 8.dp)
                )
            }
        }
    }

    dialog?.let { request ->
        NameDialog(
            request = request,
            onConfirm = { name ->
                when (request) {
                    PresetDialog.SaveNew -> {
                        repository.save(repository.capture(name))
                        activeName = name
                        message = "Preset '$name' disimpan."
                    }
                    is PresetDialog.Rename -> {
                        repository.rename(request.preset.name, name)
                        if (activeName == request.preset.name) activeName = name
                        message = "Preset diganti nama menjadi '$name'."
                    }
                    is PresetDialog.Duplicate -> {
                        repository.duplicate(request.preset, name)
                        message = "Preset diduplikasi menjadi '$name'."
                    }
                }
                refresh()
                dialog = null
            },
            onDismiss = { dialog = null }
        )
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
    onRename: () -> Unit,
    onDuplicate: () -> Unit,
    onDelete: () -> Unit
) {
    PanelCard(
        title = preset.name,
        modifier = Modifier.padding(bottom = 10.dp)
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
            Button(onClick = onLoad) { Text("Muat") }
            OutlinedButton(onClick = onRename, enabled = !preset.builtIn) { Text("Ganti Nama") }
            OutlinedButton(onClick = onDuplicate) { Text("Duplikat") }
            OutlinedButton(onClick = onDelete, enabled = !preset.builtIn) { Text("Hapus") }
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
