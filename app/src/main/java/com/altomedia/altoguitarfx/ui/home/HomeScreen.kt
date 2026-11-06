package com.altomedia.altoguitarfx.ui.home

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.altomedia.altoguitarfx.navigation.Destination
import com.altomedia.altoguitarfx.ui.components.PanelCard
import com.altomedia.altoguitarfx.ui.components.ScreenScaffold
import com.altomedia.altoguitarfx.ui.theme.TextSecondary

/**
 * Home / Pedalboard (BAB 2 kerangka). Isi lengkap (status audio, pedalboard,
 * level meter) ditambahkan pada BAB berikutnya.
 */
@Composable
fun HomeScreen(onNavigate: (Destination) -> Unit) {
    ScreenScaffold(
        title = "ALTO GUITAR FX",
        subtitle = "Pedalboard • Professional Multi Effects"
    ) {
        PanelCard(title = "Status Audio") {
            Text(
                text = "Audio engine belum aktif.",
                color = TextSecondary,
                fontSize = 13.sp
            )
        }

        PanelCard(
            title = "Pintasan",
            modifier = Modifier.padding(top = 12.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Button(onClick = { onNavigate(Destination.EFFECTS) }) {
                    Text("Efek", color = MaterialTheme.colorScheme.onPrimary)
                }
                Button(onClick = { onNavigate(Destination.PRESETS) }) {
                    Text("Preset", color = MaterialTheme.colorScheme.onPrimary)
                }
                Button(onClick = { onNavigate(Destination.TUNER) }) {
                    Text("Tuner", color = MaterialTheme.colorScheme.onPrimary)
                }
            }
        }
    }
}
