package com.altomedia.altoguitarfx.ui.components

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxScope
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import com.altomedia.altoguitarfx.R

/**
 * Latar belakang aplikasi memakai gambar brand ALTOMEDIA (res/drawable-nodpi).
 *
 * Gambar ditampilkan penuh layar (crop) dan diberi lapisan gelap (scrim) agar
 * teks serta panel tetap terbaca. Scrim lebih pekat di bagian atas dan bawah
 * tempat judul layar serta bilah navigasi berada.
 */
@Composable
fun AppBackground(
    modifier: Modifier = Modifier,
    content: @Composable BoxScope.() -> Unit
) {
    Box(modifier = modifier.fillMaxSize()) {
        Image(
            painter = painterResource(R.drawable.bg_altomedia),
            contentDescription = null,
            contentScale = ContentScale.Crop,
            alignment = Alignment.Center,
            modifier = Modifier.fillMaxSize()
        )
        // Scrim: mengurangi kontras gambar agar UI tetap terbaca, namun cukup
        // tipis agar gambar latar brand terlihat jelas (permintaan pengguna).
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(
                    Brush.verticalGradient(
                        colorStops = arrayOf(
                            0.0f to Color(0x990B0E11.toInt()),
                            0.30f to Color(0x330B0E11.toInt()),
                            0.75f to Color(0x330B0E11.toInt()),
                            1.0f to Color(0x990B0E11.toInt())
                        )
                    )
                )
        )
        content()
    }
}
