package com.altomedia.altoguitarfx.ui.effects

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.TextButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.altomedia.altoguitarfx.audio.NativeBridge
import com.altomedia.altoguitarfx.effects.EffectController
import com.altomedia.altoguitarfx.effects.EffectType
import com.altomedia.altoguitarfx.effects.ParamSpec
import com.altomedia.altoguitarfx.ui.components.PanelCard
import com.altomedia.altoguitarfx.ui.components.ScreenScaffold
import com.altomedia.altoguitarfx.ui.theme.Accent
import com.altomedia.altoguitarfx.ui.theme.TextSecondary

/**
 * Effects (BAB 7-12). Menampilkan pedal efek dengan tombol ON/OFF, kontrol
 * parameter, dan pengatur urutan (naik/turun) yang benar-benar mengubah
 * urutan pemrosesan di mesin DSP native.
 */
@Composable
fun EffectsScreen() {
    ScreenScaffold(
        title = "Effects",
        subtitle = "Rangkaian efek gitar"
    ) {
        // Urutan diambil dari mesin native sehingga UI selalu menampilkan
        // urutan pemrosesan yang sebenarnya.
        var order by remember { mutableStateOf(readOrder()) }

        PanelCard(title = "Urutan Efek") {
            Text(
                text = "Geser efek dengan tombol ▲/▼ untuk mengubah urutan " +
                    "pemrosesan. Perubahan langsung diterapkan ke DSP.",
                color = TextSecondary,
                fontSize = 12.sp
            )
            TextButton(
                onClick = {
                    NativeBridge.resetEffectOrder()
                    order = readOrder()
                },
                modifier = Modifier.padding(top = 4.dp)
            ) {
                Text("Kembalikan urutan bawaan")
            }
        }

        order.forEachIndexed { position, type ->
            EffectPanel(
                type = type,
                position = position,
                total = order.size,
                onMove = { delta ->
                    NativeBridge.moveEffect(type.id, delta)
                    order = readOrder()
                },
                modifier = Modifier.padding(bottom = 12.dp)
            )
        }
    }
}

/** Membaca urutan efek dari mesin native. */
private fun readOrder(): List<EffectType> {
    val count = NativeBridge.effectCount()
    return (0 until count).mapNotNull { slot ->
        EffectType.values().firstOrNull { it.id == NativeBridge.effectOrderAt(slot) }
    }
}

/** Satu panel pedal: judul, tombol ON/OFF, kontrol urutan, dan slider. */
@Composable
private fun EffectPanel(
    type: EffectType,
    position: Int,
    total: Int,
    onMove: (Int) -> Unit,
    modifier: Modifier = Modifier
) {
    var enabled by remember { mutableStateOf(EffectController.isEnabled(type)) }
    // Nilai awal diambil dari mesin native agar UI konsisten dengan DSP.
    var values by remember {
        mutableStateOf(
            type.params.mapIndexed { index, _ -> EffectController.param(type, index) }
        )
    }

    PanelCard(title = type.label, modifier = modifier) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = if (enabled) "AKTIF" else "BYPASS",
                color = if (enabled) Accent else TextSecondary,
                fontSize = 13.sp
            )
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(
                    text = "${position + 1}/$total",
                    color = TextSecondary,
                    fontSize = 12.sp,
                    modifier = Modifier.padding(end = 4.dp)
                )
                IconButton(
                    onClick = { onMove(-1) },
                    enabled = position > 0
                ) {
                    Text("▲", fontSize = 14.sp)
                }
                IconButton(
                    onClick = { onMove(1) },
                    enabled = position < total - 1
                ) {
                    Text("▼", fontSize = 14.sp)
                }
                Switch(
                    checked = enabled,
                    onCheckedChange = { checked ->
                        enabled = checked
                        EffectController.setEnabled(type, checked)
                    }
                )
            }
        }

        type.params.forEachIndexed { index, spec ->
            ParamSlider(
                spec = spec,
                value = values[index],
                onValueChange = { newValue ->
                    values = values.toMutableList().also { it[index] = newValue }
                    EffectController.setParam(type, index, newValue)
                }
            )
        }
    }
}

/** Slider satu parameter dengan label dan nilai berformat. */
@Composable
private fun ParamSlider(
    spec: ParamSpec,
    value: Float,
    onValueChange: (Float) -> Unit
) {
    Column(modifier = Modifier.padding(top = 8.dp)) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(text = spec.label, color = TextSecondary, fontSize = 12.sp)
            Text(
                text = formatValue(value, spec),
                color = MaterialTheme.colorScheme.onSurface,
                fontSize = 12.sp
            )
        }
        Slider(
            value = value.coerceIn(spec.min, spec.max),
            onValueChange = onValueChange,
            valueRange = spec.min..spec.max
        )
    }
}

private fun formatValue(value: Float, spec: ParamSpec): String {
    val decimals = when {
        spec.max - spec.min <= 2f -> 2
        spec.max - spec.min <= 30f -> 1
        else -> 0
    }
    val number = String.format("%.${decimals}f", value)
    return if (spec.unit.isEmpty()) number else "$number ${spec.unit}"
}
