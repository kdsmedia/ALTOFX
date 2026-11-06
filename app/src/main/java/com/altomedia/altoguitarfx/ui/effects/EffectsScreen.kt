package com.altomedia.altoguitarfx.ui.effects

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
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
import com.altomedia.altoguitarfx.effects.EffectController
import com.altomedia.altoguitarfx.effects.EffectType
import com.altomedia.altoguitarfx.effects.ParamSpec
import com.altomedia.altoguitarfx.ui.components.PanelCard
import com.altomedia.altoguitarfx.ui.components.ScreenScaffold
import com.altomedia.altoguitarfx.ui.theme.Accent
import com.altomedia.altoguitarfx.ui.theme.TextSecondary

/**
 * Effects (BAB 7). Menampilkan pedal efek dengan tombol ON/OFF dan kontrol
 * parameter yang benar-benar terhubung ke mesin DSP native.
 */
@Composable
fun EffectsScreen() {
    ScreenScaffold(
        title = "Effects",
        subtitle = "Rangkaian efek gitar"
    ) {
        // BAB 7: efek dinamika. BAB 8: efek saturasi.
        val available = remember {
            listOf(
                EffectType.NOISE_GATE,
                EffectType.COMPRESSOR,
                EffectType.OVERDRIVE,
                EffectType.DISTORTION,
                EffectType.FUZZ,
                EffectType.EQUALIZER,
                EffectType.CHORUS,
                EffectType.FLANGER,
                EffectType.PHASER,
                EffectType.TREMOLO,
                EffectType.DELAY,
                EffectType.REVERB
            )
        }

        available.forEach { type ->
            EffectPanel(type = type, modifier = Modifier.padding(bottom = 12.dp))
        }

        PanelCard(title = "Catatan") {
            Text(
                text = "Efek lain (EQ, Modulation, Delay, Reverb) " +
                    "ditambahkan pada BAB 9-11. Urutan efek tetap dan " +
                    "mengikuti alur sinyal pada rancangan.",
                color = TextSecondary,
                fontSize = 12.sp,
                modifier = Modifier.padding(top = 4.dp)
            )
        }
    }
}

/** Satu panel pedal: judul, tombol ON/OFF, dan slider tiap parameter. */
@Composable
private fun EffectPanel(type: EffectType, modifier: Modifier = Modifier) {
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
            Switch(
                checked = enabled,
                onCheckedChange = { checked ->
                    enabled = checked
                    EffectController.setEnabled(type, checked)
                }
            )
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
