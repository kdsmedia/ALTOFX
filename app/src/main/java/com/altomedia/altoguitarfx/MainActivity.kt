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
import com.altomedia.altoguitarfx.audio.AudioDeviceManager
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
    private val deviceManager by lazy { AudioDeviceManager(this) }
    private var deviceCallback: android.media.AudioDeviceCallback? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        // BAB 5/15: pulihkan pilihan perangkat & sample rate sebelum dipakai.
        AudioController.init(this)

        // BAB 15: tangani audio focus dan lifecycle. Audio dijeda saat aplikasi
        // masuk background atau kehilangan focus, lalu dilanjutkan kembali.
        audioFocus.setListener { event ->
            when (event) {
                AudioFocusController.Event.GAIN ->
                    AudioController.resumeAfterInterruption()
                AudioFocusController.Event.DUCK ->
                    AudioController.setDucked(true)
                AudioFocusController.Event.UNDUCK ->
                    AudioController.setDucked(false)
                AudioFocusController.Event.LOSS,
                AudioFocusController.Event.LOSS_TRANSIENT ->
                    AudioController.pauseForInterruption()
            }
        }
        // BAB 10/15: bila perangkat aktif dicabut, mulai ulang dengan perangkat
        // bawaan supaya audio tidak berhenti tanpa penjelasan.
        val callback = deviceManager.observe {
            AudioController.selectedInputId().let { id ->
                if (id != AudioController.DEFAULT_DEVICE_ID &&
                    deviceManager.inputDevices().none { it.id == id }
                ) {
                    AudioController.handleInputDisconnected(id)
                }
            }
            AudioController.selectedOutputId().let { id ->
                if (id != AudioController.DEFAULT_DEVICE_ID &&
                    deviceManager.outputDevices().none { it.id == id }
                ) {
                    AudioController.handleOutputDisconnected(id)
                }
            }
        }
        deviceCallback = callback

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

    override fun onDestroy() {
        deviceCallback?.let { deviceManager.stopObserving(it) }
        deviceCallback = null
        super.onDestroy()
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
