package com.altomedia.altoguitarfx.ui.components

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.altomedia.altoguitarfx.ui.theme.Accent
import com.altomedia.altoguitarfx.ui.theme.Danger
import com.altomedia.altoguitarfx.ui.theme.TextSecondary

/**
 * Level meter batang untuk satu kanal (BAB 6).
 *
 * [level] adalah nilai puncak 0..1 linier dari callback audio. Warna berubah
 * hijau -> kuning -> merah mendekati clipping.
 */
@Composable
fun LevelMeter(
    label: String,
    level: Float,
    modifier: Modifier = Modifier
) {
    val clamped = level.coerceIn(0f, 1f)
    val animated by animateFloatAsState(targetValue = clamped, label = "level")

    val barColor = when {
        clamped >= 0.95f -> Danger
        clamped >= 0.75f -> MaterialTheme.colorScheme.tertiary
        else -> Accent
    }

    Column(modifier = modifier) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(text = label, color = TextSecondary, fontSize = 12.sp)
            Text(
                text = " ${(clamped * 100).toInt()}%",
                color = TextSecondary,
                fontSize = 11.sp,
                modifier = Modifier.padding(start = 6.dp)
            )
        }
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(14.dp)
                .padding(top = 4.dp)
                .clip(RoundedCornerShape(7.dp))
                .background(MaterialTheme.colorScheme.surfaceVariant)
        ) {
            Box(
                modifier = Modifier
                    .fillMaxWidth(animated)
                    .height(14.dp)
                    .clip(RoundedCornerShape(7.dp))
                    .background(barColor)
            )
        }
    }
}
