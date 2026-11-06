package com.altomedia.altoguitarfx

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.lifecycle.DefaultLifecycleObserver
import androidx.lifecycle.LifecycleOwner
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import com.altomedia.altoguitarfx.audio.AudioController
import com.altomedia.altoguitarfx.audio.AudioFocusController
import com.altomedia.altoguitarfx.navigation.AppNavigation
import com.altomedia.altoguitarfx.navigation.Destination
import com.altomedia.altoguitarfx.navigation.switchTab
import com.altomedia.altoguitarfx.ui.theme.Accent
import com.altomedia.altoguitarfx.ui.theme.AltoGuitarFxTheme
import com.altomedia.altoguitarfx.ui.theme.PanelBg
import com.altomedia.altoguitarfx.ui.theme.TextSecondary

class MainActivity : ComponentActivity() {

    private val audioFocus by lazy { AudioFocusController(this) }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        // BAB 15: tangani audio focus dan lifecycle. Audio dijeda saat aplikasi
        // masuk background atau kehilangan focus, lalu dilanjutkan kembali.
        audioFocus.setListener { event ->
            when (event) {
                AudioFocusController.Event.GAIN,
                AudioFocusController.Event.GAIN_TRANSIENT ->
                    AudioController.resumeAfterInterruption()
                AudioFocusController.Event.LOSS,
                AudioFocusController.Event.LOSS_TRANSIENT ->
                    AudioController.pauseForInterruption()
            }
        }
        lifecycle.addObserver(object : DefaultLifecycleObserver {
            override fun onStart(owner: LifecycleOwner) {
                if (audioFocus.request()) {
                    AudioController.resumeAfterInterruption()
                }
            }

            override fun onStop(owner: LifecycleOwner) {
                AudioController.pauseForInterruption()
                audioFocus.abandon()
            }
        })

        setContent {
            AltoGuitarFxTheme {
                Surface(modifier = Modifier.fillMaxSize()) {
                    AltoGuitarFxApp()
                }
            }
        }
    }
}

@Composable
private fun AltoGuitarFxApp() {
    val navController = rememberNavController()
    val backStackEntry by navController.currentBackStackEntryAsState()
    val current = Destination.fromRoute(backStackEntry?.destination?.route)

    Scaffold(
        containerColor = MaterialTheme.colorScheme.background,
        bottomBar = {
            NavigationBar(containerColor = PanelBg) {
                Destination.entries.forEach { destination ->
                    NavigationBarItem(
                        selected = destination == current,
                        onClick = { navController.switchTab(destination) },
                        icon = { Icon(destination.icon, contentDescription = destination.label) },
                        label = { Text(destination.label) },
                        colors = NavigationBarItemDefaults.colors(
                            selectedIconColor = Accent,
                            selectedTextColor = Accent,
                            indicatorColor = PanelBg,
                            unselectedIconColor = TextSecondary,
                            unselectedTextColor = TextSecondary
                        )
                    )
                }
            }
        }
    ) { innerPadding ->
        AppNavigation(
            navController = navController,
            contentPadding = innerPadding
        )
    }
}
