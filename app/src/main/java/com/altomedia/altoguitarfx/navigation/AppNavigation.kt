package com.altomedia.altoguitarfx.navigation

import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.padding
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.navigation.NavGraph.Companion.findStartDestination
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import com.altomedia.altoguitarfx.ui.effects.EffectsScreen
import com.altomedia.altoguitarfx.ui.home.HomeScreen
import com.altomedia.altoguitarfx.ui.presets.PresetsScreen
import com.altomedia.altoguitarfx.ui.settings.SettingsScreen
import com.altomedia.altoguitarfx.ui.tuner.TunerScreen

/**
 * Grafik navigasi utama. Semua tab memakai satu NavHost agar state layar
 * terjaga saat berpindah tab.
 */
@Composable
fun AppNavigation(
    navController: NavHostController,
    contentPadding: PaddingValues,
    modifier: Modifier = Modifier
) {
    NavHost(
        navController = navController,
        startDestination = Destination.START.route,
        modifier = modifier.padding(contentPadding)
    ) {
        composable(Destination.HOME.route) {
            HomeScreen(onNavigate = { destination -> navController.switchTab(destination) })
        }
        composable(Destination.EFFECTS.route) { EffectsScreen() }
        composable(Destination.PRESETS.route) { PresetsScreen() }
        composable(Destination.TUNER.route) { TunerScreen() }
        composable(Destination.SETTINGS.route) { SettingsScreen() }
    }
}

/**
 * Berpindah tab tanpa menumpuk duplikat dan tetap menyimpan state tiap layar.
 */
fun NavHostController.switchTab(destination: Destination) {
    navigate(destination.route) {
        popUpTo(graph.findStartDestination().id) { saveState = true }
        launchSingleTop = true
        restoreState = true
    }
}
