package com.altomedia.altoguitarfx.ui.tuner

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
 * Tuner (BAB 2 kerangka). Deteksi pitch dan indikator tuning ditambahkan pada
 * BAB 14.
 */
@Composable
fun TunerScreen() {
    ScreenScaffold(
        title = "Tuner",
        subtitle = "Deteksi nada gitar"
    ) {
        PanelCard(title = "Pitch") {
            Text(
                text = "Tuner belum aktif.",
                color = TextSecondary,
                fontSize = 13.sp,
                modifier = Modifier.padding(top = 4.dp)
            )
        }
    }
}
