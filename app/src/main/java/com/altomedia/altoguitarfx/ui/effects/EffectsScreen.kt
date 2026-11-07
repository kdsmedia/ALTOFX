package com.altomedia.altoguitarfx.ui.effects

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.key
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.altomedia.altoguitarfx.audio.NativeBridge
import com.altomedia.altoguitarfx.effects.EffectController
import com.altomedia.altoguitarfx.effects.EffectType
import com.altomedia.altoguitarfx.ui.components.PanelCard
import com.altomedia.altoguitarfx.ui.components.PedalBox
import com.altomedia.altoguitarfx.ui.components.ScreenScaffold
import com.altomedia.altoguitarfx.ui.theme.TextSecondary

/**
 * Effects (BAB 7-12). Menampilkan pedalboard: setiap efek sebagai kotak pedal
 * dengan tombol ON/OFF bergaya sakelar kaki (bukan tombol geser), tombol
 * Preset cepat, dan tombol Atur untuk membuka layar pengaturan efek tersebut
 * (pengaturan per efek, bukan satu halaman gabungan).
 */
@Composable
fun EffectsScreen(onOpenEffect: (EffectType) -> Unit) {
    ScreenScaffold(
        title = "Effects",
        subtitle = "Pedalboard — ketuk pedal untuk mengatur"
    ) {
        // Urutan diambil dari mesin native sehingga UI selalu menampilkan
        // urutan pemrosesan yang sebenarnya.
        var order by remember { mutableStateOf(readOrder()) }
        var enabledMap by remember { mutableStateOf(readEnabled(order)) }
        var presetIndex by remember { mutableStateOf(emptyMap<EffectType, Int>()) }

        PanelCard(title = "Urutan Efek") {
            Text(
                text = "Urutan pemrosesan sesuai rantai nyata. Tombol ▲/▼ pada " +
                    "layar pengaturan tiap efek memindahkan posisinya.",
                color = TextSecondary,
                fontSize = 12.sp
            )
            TextButton(
                onClick = {
                    NativeBridge.resetEffectOrder()
                    order = readOrder()
                    enabledMap = readEnabled(order)
                },
                modifier = Modifier.padding(top = 4.dp)
            ) {
                Text("Kembalikan urutan bawaan")
            }
        }

        // Pedalboard dua kolom. key(type) menjaga state mengikuti efek saat
        // urutan berubah.
        order.chunked(2).forEach { rowItems ->
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(top = 12.dp),
                horizontalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                rowItems.forEach { type ->
                    key(type) {
                        val presetName = type.quickPresets
                            .getOrNull((presetIndex[type] ?: 0) % maxOf(1, type.quickPresets.size))
                            ?.name
                        PedalBox(
                            title = type.label,
                            enabled = enabledMap[type] == true,
                            badge = "${order.indexOf(type) + 1}",
                            presetLabel = presetName,
                            onToggle = { enabled ->
                                EffectController.setEnabled(type, enabled)
                                enabledMap = enabledMap.toMutableMap().also { it[type] = enabled }
                            },
                            onPreset = if (type.quickPresets.isEmpty()) null else {
                                {
                                    applyQuickPreset(type, presetIndex[type] ?: 0)
                                    presetIndex = presetIndex.toMutableMap().also {
                                        it[type] = ((presetIndex[type] ?: 0) + 1) %
                                            type.quickPresets.size
                                    }
                                }
                            },
                            onOpen = { onOpenEffect(type) },
                            modifier = Modifier.weight(1f)
                        )
                    }
                }
                if (rowItems.size == 1) {
                    Column(modifier = Modifier.weight(1f)) {}
                }
            }
        }

        Text(
            text = "LED menyala = efek aktif. Tombol OFF meneruskan sinyal apa " +
                "adanya (bypass) untuk efek itu.",
            color = TextSecondary,
            fontSize = 11.sp,
            modifier = Modifier.padding(top = 12.dp)
        )
    }
}

/** Membaca urutan efek dari mesin native. */
internal fun readOrder(): List<EffectType> {
    val count = NativeBridge.effectCount()
    return (0 until count).mapNotNull { slot ->
        EffectType.values().firstOrNull { it.id == NativeBridge.effectOrderAt(slot) }
    }
}

/** Membaca status aktif tiap efek pada urutan tertentu. */
internal fun readEnabled(order: List<EffectType>): Map<EffectType, Boolean> =
    order.associateWith { EffectController.isEnabled(it) }

/**
 * Menerapkan preset cepat satu efek: seluruh parameternya diatur sekaligus.
 * Perubahan dihaluskan di sisi DSP (SmoothedParam) sehingga tidak ada klik.
 */
internal fun applyQuickPreset(type: EffectType, index: Int) {
    val preset = type.quickPresets.getOrNull(index % maxOf(1, type.quickPresets.size))
        ?: return
    preset.values.forEach { (paramIndex, value) ->
        EffectController.setParam(type, paramIndex, value)
    }
}
