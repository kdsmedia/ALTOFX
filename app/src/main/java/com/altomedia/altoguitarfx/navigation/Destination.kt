package com.altomedia.altoguitarfx.navigation

import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.GraphicEq
import androidx.compose.material.icons.filled.Home
import androidx.compose.material.icons.filled.Save
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Tune
import androidx.compose.ui.graphics.vector.ImageVector

/**
 * Daftar tujuan navigasi utama (BAB 2).
 * Urutannya menentukan urutan tab pada bottom navigation.
 */
enum class Destination(
    val route: String,
    val label: String,
    val icon: ImageVector
) {
    HOME("home", "Home", Icons.Filled.Home),
    EFFECTS("effects", "Effects", Icons.Filled.Tune),
    PRESETS("presets", "Presets", Icons.Filled.Save),
    TUNER("tuner", "Tuner", Icons.Filled.GraphicEq),
    SETTINGS("settings", "Settings", Icons.Filled.Settings);

    companion object {
        val START = HOME

        fun fromRoute(route: String?): Destination =
            entries.firstOrNull { it.route == route } ?: START
    }
}
