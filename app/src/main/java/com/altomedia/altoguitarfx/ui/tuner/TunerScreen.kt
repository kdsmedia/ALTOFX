package com.altomedia.altoguitarfx.ui.tuner

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.OutlinedButton
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
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.altomedia.altoguitarfx.audio.AudioController
import com.altomedia.altoguitarfx.audio.NativeBridge
import com.altomedia.altoguitarfx.ui.components.PanelCard
import com.altomedia.altoguitarfx.ui.components.ScreenScaffold
import com.altomedia.altoguitarfx.ui.theme.Accent
import com.altomedia.altoguitarfx.ui.theme.TextSecondary
import kotlinx.coroutines.delay
import kotlin.math.abs
import kotlin.math.roundToInt

/** Nama nada berdasarkan nomor MIDI (0 = C-1). */
private val NOTE_NAMES = listOf(
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
)

/**
 * Tuner (BAB 14). Menampilkan hasil deteksi pitch nyata dari mesin DSP:
 * nama nada, frekuensi, selisih cents, dan indikator terlalu rendah/tinggi.
 */
@Composable
fun TunerScreen() {
    var frequency by remember { mutableStateOf(0f) }
    var midiNote by remember { mutableStateOf(-1f) }
    var cents by remember { mutableStateOf(0f) }
    var confidence by remember { mutableStateOf(0f) }
    // Referensi tuning dipulihkan dari pengaturan tersimpan (BAB 14).
    var reference by remember { mutableStateOf(AudioController.tunerReference()) }
    var referenceMenu by remember { mutableStateOf(false) }

    // Analisis hanya aktif saat layar tuner dibuka.
    DisposableEffect(Unit) {
        NativeBridge.setTunerActive(true)
        onDispose { NativeBridge.setTunerActive(false) }
    }

    // Membaca hasil deteksi secara berkala dari mesin native.
    LaunchedEffect(Unit) {
        while (true) {
            frequency = NativeBridge.tunerFrequency()
            midiNote = NativeBridge.tunerMidiNote()
            cents = NativeBridge.tunerCents()
            confidence = NativeBridge.tunerConfidence()
            delay(50)
        }
    }

    ScreenScaffold(
        title = "Tuner",
        subtitle = "Deteksi nada gitar"
    ) {
        PanelCard(title = "Referensi") {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(text = "A4 = ${reference.roundToInt()} Hz", color = TextSecondary, fontSize = 13.sp)
                OutlinedButton(onClick = { referenceMenu = true }) { Text("Ubah") }
                DropdownMenu(
                    expanded = referenceMenu,
                    onDismissRequest = { referenceMenu = false }
                ) {
                    listOf(432f, 435f, 436f, 437f, 438f, 439f, 440f, 441f, 442f, 443f, 444f, 445f)
                        .forEach { value ->
                            DropdownMenuItem(
                                text = { Text("$value Hz") },
                                onClick = {
                                    reference = value
                                    AudioController.setTunerReference(value)
                                    referenceMenu = false
                                }
                            )
                        }
                }
            }
        }

        PanelCard(title = "Pitch") {
            val hasNote = frequency > 0f && confidence > 0.3f
            val noteName = if (midiNote >= 0f) {
                val index = midiNote.roundToInt().floorMod(12)
                val octave = midiNote.roundToInt().floorDiv(12) - 1
                "${NOTE_NAMES[index]}$octave"
            } else {
                "--"
            }
            val inTune = abs(cents) < 5f

            Text(
                text = noteName,
                color = if (hasNote) Accent else TextSecondary,
                fontSize = 48.sp,
                fontWeight = FontWeight.Bold
            )
            Text(
                text = if (hasNote) {
                    "${"%.2f".format(frequency)} Hz"
                } else {
                    "Mainkan satu senar gitar"
                },
                color = TextSecondary,
                fontSize = 14.sp
            )
            Text(
                text = when {
                    !hasNote -> "Menunggu nada..."
                    inTune -> "SELARAS"
                    cents < 0f -> "Terlalu rendah"
                    else -> "Terlalu tinggi"
                },
                color = if (hasNote && inTune) Accent else TextSecondary,
                fontSize = 14.sp,
                fontWeight = FontWeight.Bold,
                modifier = Modifier.padding(top = 4.dp)
            )
            Text(
                text = if (hasNote) "${"%.0f".format(cents)} cents" else "",
                color = TextSecondary,
                fontSize = 12.sp
            )

            // Jarum indikator: 0 di tengah, batas -50..+50 cents.
            CentsMeter(
                cents = if (hasNote) cents else 0f,
                active = hasNote,
                modifier = Modifier
                    .fillMaxWidth()
                    .height(64.dp)
                    .padding(top = 8.dp)
            )
        }
    }
}

/** Jarum tuning sederhana yang digambar dengan Canvas. */
@Composable
private fun CentsMeter(cents: Float, active: Boolean, modifier: Modifier = Modifier) {
    val accent = Accent
    val secondary = TextSecondary
    Canvas(modifier = modifier) {
        val centerX = size.width / 2f
        val bottom = size.height
        // Garis skala.
        drawLine(
            color = secondary,
            start = Offset(centerX, 0f),
            end = Offset(centerX, bottom),
            strokeWidth = 2f
        )
        // Penanda tengah (selaras).
        drawLine(
            color = accent,
            start = Offset(centerX - 12f, bottom * 0.3f),
            end = Offset(centerX + 12f, bottom * 0.3f),
            strokeWidth = 4f
        )
        if (active) {
            val clamped = cents.coerceIn(-50f, 50f)
            val x = centerX + (clamped / 50f) * (size.width / 2f - 8f)
            drawLine(
                color = accent,
                start = Offset(centerX, bottom),
                end = Offset(x, bottom * 0.15f),
                strokeWidth = 6f
            )
        }
    }
}

private fun Int.floorMod(modulus: Int): Int = ((this % modulus) + modulus) % modulus

private fun Int.floorDiv(divisor: Int): Int {
    val q = this / divisor
    return if ((this % divisor != 0) && ((this < 0) != (divisor < 0))) q - 1 else q
}
