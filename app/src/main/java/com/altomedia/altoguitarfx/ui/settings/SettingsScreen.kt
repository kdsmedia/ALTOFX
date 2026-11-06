package com.altomedia.altoguitarfx.ui.settings

import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.altomedia.altoguitarfx.ui.components.PanelCard
import com.altomedia.altoguitarfx.ui.components.ScreenScaffold
import com.altomedia.altoguitarfx.ui.theme.TextSecondary

/**
 * Settings (BAB 2 kerangka). Pemilihan perangkat, sample rate, buffer, izin,
 * dan info aplikasi ditambahkan pada BAB 5 dan 15.
 */
@Composable
fun SettingsScreen() {
    ScreenScaffold(
        title = "Settings",
        subtitle = "Perangkat dan performa audio"
    ) {
        PanelCard(title = "Perangkat Audio") {
            Text(
                text = "Pemilihan input/output belum tersedia.",
                color = TextSecondary,
                fontSize = 13.sp,
                modifier = Modifier.padding(top = 4.dp)
            )
        }
        PanelCard(
            title = "Tentang",
            modifier = Modifier.padding(top = 12.dp)
        ) {
            Text(
                text = "ALTO GUITAR FX • ALTOMEDIA\nVersi 1.0 (rancangan)",
                color = TextSecondary,
                fontSize = 13.sp
            )
        }
    }
}
