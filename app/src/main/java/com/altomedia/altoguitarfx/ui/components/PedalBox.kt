package com.altomedia.altoguitarfx.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.altomedia.altoguitarfx.ui.theme.Accent
import com.altomedia.altoguitarfx.ui.theme.AccentDark
import com.altomedia.altoguitarfx.ui.theme.PanelBgAlt
import com.altomedia.altoguitarfx.ui.theme.TextSecondary

/**
 * Kotak pedal bergaya pedal fisik (BAB 7-12).
 *
 * Menampilkan nama efek, LED indikator, tombol ON/OFF berbentuk sakelar kaki
 * (footswitch), serta tombol "Preset" (preset cepat per efek) dan "Atur"
 * (membuka layar pengaturan efek tersebut). Seluruh kotak dapat diklik untuk
 * membuka pengaturan.
 */
@Composable
fun PedalBox(
    title: String,
    enabled: Boolean,
    onToggle: (Boolean) -> Unit,
    onPreset: (() -> Unit)? = null,
    presetLabel: String? = null,
    onOpen: (() -> Unit)? = null,
    badge: String? = null,
    modifier: Modifier = Modifier
) {
    val borderColor = if (enabled) Accent.copy(alpha = 0.7f) else Color(0xFF2A313A)
    Column(
        modifier = modifier
            .clip(RoundedCornerShape(16.dp))
            .background(PanelBgAlt)
            .border(1.dp, borderColor, RoundedCornerShape(16.dp))
            .clickable(enabled = onOpen != null) { onOpen?.invoke() }
            .padding(horizontal = 12.dp, vertical = 12.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = title,
                color = MaterialTheme.colorScheme.onSurface,
                fontSize = 14.sp,
                fontWeight = FontWeight.SemiBold,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
                modifier = Modifier.weight(1f)
            )
            if (badge != null) {
                Text(text = badge, color = TextSecondary, fontSize = 11.sp)
            }
        }

        Spacer(Modifier.height(10.dp))
        // LED indikator: menyala saat efek aktif.
        Box(
            modifier = Modifier
                .size(10.dp)
                .clip(CircleShape)
                .background(if (enabled) Accent else Color(0xFF3A3F45))
        )
        Spacer(Modifier.height(8.dp))

        FootSwitch(enabled = enabled, onClick = { onToggle(!enabled) })

        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(top = 6.dp),
            horizontalArrangement = Arrangement.SpaceEvenly,
            verticalAlignment = Alignment.CenterVertically
        ) {
            if (onPreset != null) {
                TextButton(onClick = onPreset, contentPadding = TextButtonPadding) {
                    Text(
                        text = presetLabel?.let { "Preset: $it" } ?: "Preset",
                        fontSize = 11.sp,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis
                    )
                }
            }
            if (onOpen != null) {
                TextButton(onClick = onOpen, contentPadding = TextButtonPadding) {
                    Text("Atur", fontSize = 11.sp, color = Accent)
                }
            }
        }
    }
}

/** Sakelar kaki bulat bergaya pedal: menyala penuh saat ON. */
@Composable
private fun FootSwitch(enabled: Boolean, onClick: () -> Unit) {
    Box(
        modifier = Modifier
            .size(64.dp)
            .clip(CircleShape)
            .background(if (enabled) Accent else AccentDark.copy(alpha = 0.35f))
            .border(
                width = 3.dp,
                color = if (enabled) Accent else Color(0xFF3A3F45),
                shape = CircleShape
            )
            .clickable(onClick = onClick),
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = if (enabled) "ON" else "OFF",
            color = if (enabled) Color.Black else TextSecondary,
            fontSize = 14.sp,
            fontWeight = FontWeight.Bold,
            textAlign = TextAlign.Center
        )
    }
}

private val TextButtonPadding =
    androidx.compose.foundation.layout.PaddingValues(horizontal = 8.dp, vertical = 2.dp)
