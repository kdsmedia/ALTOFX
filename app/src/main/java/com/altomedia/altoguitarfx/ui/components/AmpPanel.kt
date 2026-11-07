package com.altomedia.altoguitarfx.ui.components

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.altomedia.altoguitarfx.effects.EffectController
import com.altomedia.altoguitarfx.effects.EffectType
import com.altomedia.altoguitarfx.ui.theme.Accent
import com.altomedia.altoguitarfx.ui.theme.Danger
import com.altomedia.altoguitarfx.ui.theme.PanelBgAlt
import com.altomedia.altoguitarfx.ui.theme.TextSecondary

/**
 * Panel amplifier bergaya Tonebridge: pemilih voicing (Clean/Crunch/Lead) dan
 * deretan knob Gain, Bass, Mid, Treble, Presence, Master.
 *
 * Seluruh nilai dibaca/ditulis ke mesin DSP lewat [EffectController]; perubahan
 * dihaluskan di sisi native (SmoothedParam) sehingga tidak ada klik.
 */
@Composable
fun AmpPanel(modifier: Modifier = Modifier) {
    val type = EffectType.AMP
    var enabled by remember { mutableStateOf(EffectController.isEnabled(type)) }
    var values by remember {
        mutableStateOf(type.params.indices.map { EffectController.param(type, it) })
    }

    fun setParam(index: Int, value: Float) {
        EffectController.setParam(type, index, value)
        values = values.toMutableList().also { it[index] = value }
    }

    PanelCard(title = "Amp", modifier = modifier) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            OutlinedButton(
                onClick = {
                    enabled = !enabled
                    EffectController.setEnabled(type, enabled)
                },
                colors = if (enabled) {
                    ButtonDefaults.outlinedButtonColors(contentColor = Accent)
                } else {
                    ButtonDefaults.outlinedButtonColors(contentColor = Danger)
                }
            ) {
                Text(if (enabled) "AMP: ON" else "AMP: OFF", fontSize = 13.sp)
            }
            Text(
                text = "Model amplifier",
                color = TextSecondary,
                fontSize = 12.sp
            )
        }

        // Pemilih voicing: karakter amplifier.
        Text(
            text = "Voicing",
            color = TextSecondary,
            fontSize = 12.sp,
            modifier = Modifier.padding(top = 10.dp)
        )
        val voicingNames = listOf("Clean", "Crunch", "Lead")
        val selectedVoicing = (values.getOrNull(0) ?: 1f).toInt().coerceIn(0, 2)
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(top = 6.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            voicingNames.forEachIndexed { index, name ->
                val selected = selectedVoicing == index
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .clip(RoundedCornerShape(10.dp))
                        .background(if (selected) Accent else PanelBgAlt)
                        .border(
                            BorderStroke(1.dp, if (selected) Accent else Color(0xFF2A313A)),
                            RoundedCornerShape(10.dp)
                        )
                        .clickable(enabled = enabled) { setParam(0, index.toFloat()) }
                        .padding(vertical = 8.dp),
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        text = name,
                        color = if (selected) Color.Black else MaterialTheme.colorScheme.onSurface,
                        fontSize = 13.sp,
                        fontWeight = FontWeight.SemiBold
                    )
                }
            }
        }

        // Deretan knob parameter (2 baris x 3).
        val knobs = listOf(
            1 to "Gain",
            2 to "Bass",
            3 to "Mid",
            4 to "Treble",
            5 to "Presence",
            6 to "Master"
        )
        knobs.chunked(3).forEach { rowItems ->
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(top = 14.dp),
                horizontalArrangement = Arrangement.SpaceEvenly
            ) {
                rowItems.forEach { (index, label) ->
                    val spec = type.params[index]
                    Knob(
                        label = label,
                        value = values.getOrNull(index) ?: spec.default,
                        range = spec.min..spec.max,
                        accent = if (enabled) Accent else TextSecondary,
                        valueText = formatValue(values.getOrNull(index) ?: spec.default, spec.unit),
                        onChange = { setParam(index, it) }
                    )
                }
            }
        }

        if (!enabled) {
            Text(
                text = "Amp sedang nonaktif. Nyalakan untuk mendengar karakter " +
                    "amplifier.",
                color = TextSecondary,
                fontSize = 11.sp,
                modifier = Modifier.padding(top = 10.dp)
            )
        }
    }
}

/** Memformat nilai knob: tanpa desimal untuk satuan kosong, satu untuk dB/Hz. */
private fun formatValue(value: Float, unit: String): String {
    return if (unit.isEmpty()) {
        "%.2f".format(value)
    } else {
        "%.1f%s".format(value, unit)
    }
}
