package com.altomedia.altoguitarfx.ui.effects

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
 * Effects (BAB 2 kerangka). Daftar efek, tombol ON/OFF, dan kontrol parameter
 * ditambahkan pada BAB 7-12.
 */
@Composable
fun EffectsScreen() {
    ScreenScaffold(
        title = "Effects",
        subtitle = "Rangkaian efek gitar"
    ) {
        PanelCard(title = "Daftar Efek") {
            Text(
                text = "Katalog efek dan kontrol parameter belum tersedia.",
                color = TextSecondary,
                fontSize = 13.sp,
                modifier = Modifier.padding(top = 4.dp)
            )
        }
    }
}
