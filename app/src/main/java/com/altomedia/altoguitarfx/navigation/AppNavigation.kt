package com.altomedia.altoguitarfx.navigation

import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.padding
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.navigation.NavGraph.Companion.findStartDestination
import androidx.navigation.NavHostController
import androidx.navigation.NavType
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.navArgument
import com.altomedia.altoguitarfx.ui.effects.EffectDetailScreen
import com.altomedia.altoguitarfx.ui.effects.EffectsScreen
import com.altomedia.altoguitarfx.ui.home.HomeScreen
import com.altomedia.altoguitarfx.ui.presets.PresetsScreen
import com.altomedia.altoguitarfx.ui.settings.SettingsScreen
import com.altomedia.altoguitarfx.ui.tuner.TunerScreen

/** Rute layar pengaturan satu efek (BAB 12): pengaturan terbuka per efek. */
const val EFFECT_DETAIL_ROUTE = "effect/{effectId}"

fun effectDetailRoute(effectId: Int): String = "effect/$effectId"

/**
 * Grafik navigasi utama. Semua tab memakai satu NavHost agar state layar
 * terjaga saat berpindah tab. Layar pengaturan per efek dibuka di atas tab
 * Effects (bukan menggabungkan semua efek pada satu halaman).
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
        composable(Destination.EFFECTS.route) {
            EffectsScreen(
                onOpenEffect = { type -> navController.navigate(effectDetailRoute(type.id)) }
            )
        }
        composable(
            route = EFFECT_DETAIL_ROUTE,
            arguments = listOf(navArgument("effectId") { type = NavType.IntType })
        ) { entry ->
            val effectId = entry.arguments?.getInt("effectId") ?: 0
            EffectDetailScreen(
                effectId = effectId,
                onBack = { navController.popBackStack() }
            )
        }
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
