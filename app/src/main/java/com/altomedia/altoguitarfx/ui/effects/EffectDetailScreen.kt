package com.altomedia.altoguitarfx.ui.effects

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.altomedia.altoguitarfx.effects.EffectController
import com.altomedia.altoguitarfx.effects.EffectType
import com.altomedia.altoguitarfx.effects.ParamSpec
import com.altomedia.altoguitarfx.ui.components.Knob
import com.altomedia.altoguitarfx.ui.components.PanelCard
import com.altomedia.altoguitarfx.ui.components.ScreenScaffold
import com.altomedia.altoguitarfx.ui.theme.Accent
import com.altomedia.altoguitarfx.ui.theme.Danger
import com.altomedia.altoguitarfx.ui.theme.TextSecondary

/**
 * Layar pengaturan SATU efek (BAB 12). Dibuka dari pedalboard sehingga setiap
 * efek memiliki halamannya sendiri: tombol ON/OFF, kontrol parameter, preset
 * cepat, pemindahan urutan, dan tombol reset ke nilai bawaan.
 */
@Composable
fun EffectDetailScreen(effectId: Int, onBack: () -> Unit) {
    val type = EffectType.values().firstOrNull { it.id == effectId } ?: run {
        ScreenScaffold(title = "Efek tidak dikenal") {
            TextButton(onClick = onBack) { Text("Kembali") }
        }
        return
    }

    var enabled by remember { mutableStateOf(EffectController.isEnabled(type)) }
    var values by remember {
        mutableStateOf(
            type.params.mapIndexed { index, _ -> EffectController.param(type, index) }
        )
    }
    val order = remember { readOrder() }
    val position = order.indexOf(type).coerceAtLeast(0)

    ScreenScaffold(
        title = type.label,
        subtitle = "Pengaturan efek"
    ) {
        // Tombol kembali + status ON/OFF besar.
        PanelCard {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedButton(onClick = onBack) { Text("Kembali") }
                Text(
                    text = if (enabled) "AKTIF" else "BYPASS",
                    color = if (enabled) Accent else TextSecondary,
                    fontSize = 15.sp
                )
            }
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(top = 12.dp),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                OutlinedButton(
                    onClick = {
                        enabled = !enabled
                        EffectController.setEnabled(type, enabled)
                    },
                    modifier = Modifier.weight(1f)
                ) {
                    Text(
                        text = if (enabled) "Matikan (Bypass)" else "Nyalakan",
                        color = if (enabled) Danger else Accent
                    )
                }
                OutlinedButton(
                    onClick = {
                        EffectController.applyDefaults(type)
                        values = type.params.map { it.default }
                    },
                    modifier = Modifier.weight(1f)
                ) {
                    Text("Reset")
                }
            }
        }

        // Preset cepat khusus efek ini.
        if (type.quickPresets.isNotEmpty()) {
            PanelCard(
                title = "Preset Cepat",
                modifier = Modifier.padding(top = 12.dp)
            ) {
                Text(
                    text = "Pilih karakter efek dengan satu ketukan.",
                    color = TextSecondary,
                    fontSize = 12.sp
                )
                type.quickPresets.forEach { preset ->
                    OutlinedButton(
                        onClick = {
                            preset.values.forEach { (index, value) ->
                                EffectController.setParam(type, index, value)
                            }
                            values = type.params.mapIndexed { index, spec ->
                                preset.values[index] ?: spec.default
                            }
                        },
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(top = 6.dp)
                    ) {
                        Text(preset.name)
                    }
                }
            }
        }

        // Kontrol parameter efek ini saja, disusun sebagai deretan knob
        // bergaya panel amplifier (Tonebridge).
        PanelCard(
            title = "Parameter",
            modifier = Modifier.padding(top = 12.dp)
        ) {
            Text(
                text = "Geser knob ke atas/bawah untuk mengubah nilai.",
                color = TextSecondary,
                fontSize = 12.sp
            )
            type.params.indices.chunked(3).forEach { rowIndices ->
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(top = 14.dp),
                    horizontalArrangement = Arrangement.SpaceEvenly
                ) {
                    rowIndices.forEach { index ->
                        val spec = type.params[index]
                        Knob(
                            label = spec.label,
                            value = values[index],
                            range = spec.min..spec.max,
                            accent = if (enabled) Accent else TextSecondary,
                            valueText = formatValue(values[index], spec),
                            onChange = { newValue ->
                                values = values.toMutableList().also { it[index] = newValue }
                                EffectController.setParam(type, index, newValue)
                            }
                        )
                    }
                    // Isi sisa baris agar knob tetap rata kiri.
                    repeat(3 - rowIndices.size) {
                        Column(modifier = Modifier.weight(1f)) {}
                    }
                }
            }
        }

        // Pindah posisi dalam rantai (urutan pemrosesan).
        PanelCard(
            title = "Posisi dalam Rantai",
            modifier = Modifier.padding(top = 12.dp)
        ) {
            Text(
                text = "Posisi saat ini: ${position + 1} dari ${order.size}",
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
                    onClick = { com.altomedia.altoguitarfx.audio.NativeBridge.moveEffect(type.id, -1) },
                    enabled = position > 0,
                    modifier = Modifier.weight(1f)
                ) {
                    Text("▲ Naikkan")
                }
                OutlinedButton(
                    onClick = { com.altomedia.altoguitarfx.audio.NativeBridge.moveEffect(type.id, 1) },
                    enabled = position < order.size - 1,
                    modifier = Modifier.weight(1f)
                ) {
                    Text("▼ Turunkan")
                }
            }
        }
    }
}

/** Memformat nilai parameter: jumlah desimal mengikuti lebar rentang. */
private fun formatValue(value: Float, spec: ParamSpec): String {
    val decimals = when {
        spec.max - spec.min <= 2f -> 2
        spec.max - spec.min <= 30f -> 1
        else -> 0
    }
    val number = String.format("%.${decimals}f", value)
    return if (spec.unit.isEmpty()) number else "$number ${spec.unit}"
}
