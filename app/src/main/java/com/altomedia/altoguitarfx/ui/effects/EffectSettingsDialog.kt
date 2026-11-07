package com.altomedia.altoguitarfx.ui.effects

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
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
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import com.altomedia.altoguitarfx.audio.NativeBridge
import com.altomedia.altoguitarfx.effects.EffectController
import com.altomedia.altoguitarfx.effects.EffectType
import com.altomedia.altoguitarfx.effects.ParamSpec
import com.altomedia.altoguitarfx.ui.components.Knob
import com.altomedia.altoguitarfx.ui.theme.Accent
import com.altomedia.altoguitarfx.ui.theme.Danger
import com.altomedia.altoguitarfx.ui.theme.PanelBgAlt
import com.altomedia.altoguitarfx.ui.theme.TextSecondary

/**
 * Popup pengaturan SATU efek (BAB 12). Dibuka dari kotak pedal sehingga
 * pengaturan muncul sebagai jendela di atas pedalboard — tidak perlu berpindah
 * halaman. Berisi status ON/OFF, preset cepat, knob parameter, dan pemindahan
 * urutan dalam rantai.
 */
@Composable
fun EffectSettingsDialog(effectId: Int, onDismiss: () -> Unit) {
    val type = EffectType.values().firstOrNull { it.id == effectId } ?: return
    var enabled by remember(type) { mutableStateOf(EffectController.isEnabled(type)) }
    var values by remember(type) {
        mutableStateOf(type.params.indices.map { EffectController.param(type, it) })
    }
    var order by remember(type) { mutableStateOf(readOrder()) }
    val position = order.indexOf(type).coerceAtLeast(0)
    var noise by remember(type) { mutableStateOf(EffectController.isNoise(type)) }

    Dialog(
        onDismissRequest = onDismiss,
        properties = DialogProperties(usePlatformDefaultWidth = false)
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth(0.92f)
                .background(PanelBgAlt.copy(alpha = 0.92f), RoundedCornerShape(18.dp))
                .padding(16.dp)
        ) {
            // Judul + status.
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = type.label,
                    color = MaterialTheme.colorScheme.onSurface,
                    fontSize = 19.sp,
                    fontWeight = FontWeight.Bold
                )
                Text(
                    text = if (enabled) "AKTIF" else "BYPASS",
                    color = if (enabled) Accent else TextSecondary,
                    fontSize = 13.sp,
                    fontWeight = FontWeight.SemiBold
                )
            }

            // Isi dialog dapat di-scroll bila parameter banyak.
            Column(
                modifier = Modifier
                    .heightIn(max = 460.dp)
                    .verticalScroll(rememberScrollState())
                    .padding(top = 10.dp)
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    OutlinedButton(
                        onClick = {
                            enabled = !enabled
                            EffectController.setEnabled(type, enabled)
                        },
                        modifier = Modifier.weight(1f),
                        colors = if (enabled) {
                            ButtonDefaults.outlinedButtonColors(contentColor = Danger)
                        } else {
                            ButtonDefaults.outlinedButtonColors(contentColor = Accent)
                        }
                    ) {
                        Text(if (enabled) "Matikan (Bypass)" else "Nyalakan")
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

                // Tombol NOISE per efek: pembersih bising pada jalur efek ini.
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(top = 10.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Column(modifier = Modifier.weight(1f)) {
                        Text(
                            text = "Noise",
                            color = MaterialTheme.colorScheme.onSurface,
                            fontSize = 14.sp,
                            fontWeight = FontWeight.SemiBold
                        )
                        Text(
                            text = "Peredam hiss/derau sebelum efek ini",
                            color = TextSecondary,
                            fontSize = 11.sp
                        )
                    }
                    OutlinedButton(
                        onClick = {
                            noise = !noise
                            EffectController.setNoise(type, noise)
                        },
                        colors = if (noise) {
                            ButtonDefaults.outlinedButtonColors(contentColor = Accent)
                        } else {
                            ButtonDefaults.outlinedButtonColors(contentColor = TextSecondary)
                        }
                    ) {
                        Text(if (noise) "NOISE: ON" else "NOISE: OFF")
                    }
                }

                // Preset cepat khusus efek ini.
                if (type.quickPresets.isNotEmpty()) {
                    Text(
                        text = "Preset Cepat",
                        color = MaterialTheme.colorScheme.onSurface,
                        fontSize = 14.sp,
                        fontWeight = FontWeight.SemiBold,
                        modifier = Modifier.padding(top = 14.dp)
                    )
                    type.quickPresets.chunked(2).forEach { rowItems ->
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(top = 6.dp),
                            horizontalArrangement = Arrangement.spacedBy(8.dp)
                        ) {
                            rowItems.forEach { preset ->
                                OutlinedButton(
                                    onClick = {
                                        preset.values.forEach { (index, value) ->
                                            EffectController.setParam(type, index, value)
                                        }
                                        values = type.params.mapIndexed { index, spec ->
                                            preset.values[index] ?: spec.default
                                        }
                                    },
                                    modifier = Modifier.weight(1f)
                                ) {
                                    Text(preset.name, fontSize = 13.sp)
                                }
                            }
                            if (rowItems.size == 1) {
                                Column(modifier = Modifier.weight(1f)) {}
                            }
                        }
                    }
                }

                // Knob parameter.
                Text(
                    text = "Parameter",
                    color = MaterialTheme.colorScheme.onSurface,
                    fontSize = 14.sp,
                    fontWeight = FontWeight.SemiBold,
                    modifier = Modifier.padding(top = 14.dp)
                )
                type.params.indices.chunked(3).forEach { rowIndices ->
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(top = 12.dp),
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
                        repeat(3 - rowIndices.size) {
                            Column(modifier = Modifier.weight(1f)) {}
                        }
                    }
                }

                // Posisi dalam rantai.
                Text(
                    text = "Posisi dalam Rantai",
                    color = MaterialTheme.colorScheme.onSurface,
                    fontSize = 14.sp,
                    fontWeight = FontWeight.SemiBold,
                    modifier = Modifier.padding(top = 16.dp)
                )
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
                        onClick = {
                            NativeBridge.moveEffect(type.id, -1)
                            order = readOrder()
                        },
                        enabled = position > 0,
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("▲ Naikkan")
                    }
                    OutlinedButton(
                        onClick = {
                            NativeBridge.moveEffect(type.id, 1)
                            order = readOrder()
                        },
                        enabled = position < order.size - 1,
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("▼ Turunkan")
                    }
                }
            }

            Button(
                onClick = onDismiss,
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(top = 14.dp)
            ) {
                Text("Selesai", color = Color.Black)
            }
        }
    }
}

/** Memformat nilai parameter: jumlah desimal mengikuti lebar rentang. */
internal fun formatValue(value: Float, spec: ParamSpec): String {
    val decimals = when {
        spec.max - spec.min <= 2f -> 2
        spec.max - spec.min <= 30f -> 1
        else -> 0
    }
    val number = String.format("%.${decimals}f", value)
    return if (spec.unit.isEmpty()) number else "$number ${spec.unit}"
}
