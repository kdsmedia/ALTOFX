package com.altomedia.altoguitarfx.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.detectVerticalDragGestures
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.IntSize
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.altomedia.altoguitarfx.ui.theme.Accent
import com.altomedia.altoguitarfx.ui.theme.PanelBgAlt
import com.altomedia.altoguitarfx.ui.theme.TextSecondary
import kotlin.math.PI
import kotlin.math.cos
import kotlin.math.sin

/**
 * Knob putar bergaya panel amplifier (seperti Tonebridge).
 *
 * Nilai diubah dengan menggeser vertikal di atas knob; gerakan ke atas
 * menaikkan nilai. Busur menampilkan posisi nilai dalam rentang, dan titik di
 * tepi menandai posisi saat ini. Perubahan diteruskan lewat [onChange] dan
 * dihaluskan di sisi DSP.
 */
@Composable
fun Knob(
    label: String,
    value: Float,
    range: ClosedFloatingPointRange<Float>,
    onChange: (Float) -> Unit,
    modifier: Modifier = Modifier,
    valueText: String? = null,
    accent: Color = Accent,
    diameter: Dp = 66.dp
) {
    val span = (range.endInclusive - range.start).takeIf { it != 0f } ?: 1f
    val fraction = ((value - range.start) / span).coerceIn(0f, 1f)
    var knobSize by remember { mutableStateOf(IntSize.Zero) }

    // Nilai terbaru dibaca di dalam gesture agar drag tidak memakai nilai basi
    // (pointerInput tidak restart saat nilai berubah).
    val latestValue by rememberUpdatedState(value)
    val latestRange by rememberUpdatedState(range)

    Column(
        modifier = modifier,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Canvas(
            modifier = Modifier
                .size(diameter)
                .onSizeChanged { knobSize = it }
                .pointerInput(Unit) {
                    detectVerticalDragGestures { _, dragAmount ->
                        val height = knobSize.height.coerceAtLeast(1)
                        val delta = -dragAmount / height * span * 0.9f
                        val next = (latestValue + delta).coerceIn(
                            latestRange.start,
                            latestRange.endInclusive
                        )
                        onChange(next)
                    }
                }
        ) {
            val stroke = 9.dp.toPx()
            val startAngle = 135f
            val sweep = 270f
            val inset = stroke / 2f
            val arcSize = Size(size.width - stroke, size.height - stroke)
            val topLeft = Offset(inset, inset)
            val center = Offset(size.width / 2f, size.height / 2f)

            // Badan knob gelap di bagian dalam.
            drawCircle(
                color = PanelBgAlt,
                radius = (size.minDimension / 2f) - stroke * 1.6f,
                center = center
            )
            // Lintasan busur.
            drawArc(
                color = Color(0xFF2A313A),
                startAngle = startAngle,
                sweepAngle = sweep,
                useCenter = false,
                topLeft = topLeft,
                size = arcSize,
                style = Stroke(width = stroke, cap = StrokeCap.Round)
            )
            // Busur nilai.
            drawArc(
                color = accent,
                startAngle = startAngle,
                sweepAngle = sweep * fraction,
                useCenter = false,
                topLeft = topLeft,
                size = arcSize,
                style = Stroke(width = stroke, cap = StrokeCap.Round)
            )
            // Titik penunjuk pada tepi.
            val angle = (startAngle + sweep * fraction) * (PI / 180.0)
            val radius = (size.minDimension / 2f) - stroke * 1.1f
            drawCircle(
                color = accent,
                radius = stroke * 0.62f,
                center = Offset(
                    center.x + (radius * cos(angle)).toFloat(),
                    center.y + (radius * sin(angle)).toFloat()
                )
            )
        }
        Text(
            text = label.uppercase(),
            color = TextSecondary,
            fontSize = 10.sp,
            fontWeight = FontWeight.SemiBold,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
            modifier = Modifier.padding(top = 4.dp)
        )
        if (valueText != null) {
            Text(
                text = valueText,
                color = Color(0xFFECEFF3),
                fontSize = 11.sp,
                fontWeight = FontWeight.Medium
            )
        }
    }
}
