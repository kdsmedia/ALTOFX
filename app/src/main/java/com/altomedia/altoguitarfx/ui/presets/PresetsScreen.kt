package com.altomedia.altoguitarfx.ui.presets

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
 * Presets (BAB 2 kerangka). Simpan/muat/rename/duplikasi/hapus ditambahkan
 * pada BAB 13.
 */
@Composable
fun PresetsScreen() {
    ScreenScaffold(
        title = "Presets",
        subtitle = "Preset bawaan dan preset pengguna"
    ) {
        PanelCard(title = "Daftar Preset") {
            Text(
                text = "Sistem preset belum aktif.",
                color = TextSecondary,
                fontSize = 13.sp,
                modifier = Modifier.padding(top = 4.dp)
            )
        }
    }
}
