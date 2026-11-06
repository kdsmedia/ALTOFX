package com.altomedia.altoguitarfx

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import com.altomedia.altoguitarfx.ui.theme.AltoGuitarFxTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            AltoGuitarFxTheme {
                Surface(modifier = Modifier.fillMaxSize()) {
                    Bab1Placeholder()
                }
            }
        }
    }
}

// BAB 1: titik masuk minimal. Navigasi dan layar ditambahkan pada BAB 2.
@Composable
private fun Bab1Placeholder() {
    Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
        Text(text = "ALTO GUITAR FX")
    }
}
