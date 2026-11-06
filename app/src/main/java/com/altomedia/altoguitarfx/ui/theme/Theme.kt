package com.altomedia.altoguitarfx.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

private val AltoDarkColors = darkColorScheme(
    primary = Accent,
    onPrimary = Color.Black,
    secondary = AccentDark,
    onSecondary = Color.Black,
    background = BlackBg,
    onBackground = TextPrimary,
    surface = PanelBg,
    onSurface = TextPrimary,
    surfaceVariant = PanelBgAlt,
    onSurfaceVariant = TextSecondary,
    error = Danger,
    onError = Color.Black
)

@Composable
fun AltoGuitarFxTheme(
    darkTheme: Boolean = isSystemInDarkTheme(),
    content: @Composable () -> Unit
) {
    // Aplikasi selalu gelap: pedalboard profesional tidak cocok dengan tema terang.
    MaterialTheme(
        colorScheme = AltoDarkColors,
        content = content
    )
}
