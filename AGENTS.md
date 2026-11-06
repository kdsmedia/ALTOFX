# AGENTS.md — Memori Proyek ALTO GUITAR FX

Panduan kerja untuk agen yang melanjutkan proyek ini. Baca sebelum mengubah kode.

## Ringkasan proyek
Aplikasi Android **real-time guitar multi effects** (prosesor efek gitar digital).
Blue print lengkap ada di `ALTO_GUITAR_FX_Rancangan_Lengkap.pdf` (12 halaman).
Dokumen itu adalah sumber kebenaran rancangan: arsitektur, katalog efek, preset,
spesifikasi audio, 17 BAB pembangunan, pengujian, dan checklist rilis.

- Nama aplikasi: **ALTO GUITAR FX**
- Brand/pengembang: **ALTOMEDIA**
- Package name: `com.altomedia.altoguitarfx`
- Kategori: Professional Multi Effects, mode real-time
- Bahasa: Kotlin (UI/aplikasi) + C++ (DSP native)
- UI: Jetpack Compose, tema gelap profesional
- Audio engine: Oboe + Android NDK/CMake
- Target sample rate: 48 kHz (bila didukung perangkat)

## Lingkungan build (sudah disiapkan di container ini)
Toolchain TIDAK terpasang secara default. Pasang sekali, lalu sumberkan env:

```
source ~/.android_env.sh      # JAVA_HOME, ANDROID_HOME, PATH
```

Yang terpasang: JDK 21 (`/usr/lib/jvm/java-21-openjdk-amd64`), Android SDK
(`~/android-sdk`), NDK `26.1.10909125`, CMake `3.22.1`, platform `android-34`,
build-tools `34.0.0`, Gradle `8.9` (`/opt/gradle/gradle-8.9`).

Build & verifikasi:

```
source ~/.android_env.sh
./gradlew assembleDebug            # hasil: app/build/outputs/apk/debug/app-debug.apk
```

Selalu jalankan `assembleDebug` setelah setiap perubahan agar kesalahan ketahuan dini.
`local.properties` berisi `sdk.dir` dan tidak di-commit (lihat `.gitignore`).

## Versi yang dipakai (jangan diubah tanpa alasan)
- AGP 8.5.2, Kotlin 1.9.24, Compose Compiler 1.5.14, Compose BOM 2024.06.00
- compileSdk 34, targetSdk 34, minSdk 26
- `android.useAndroidX=true` wajib ada di `gradle.properties`

## Aturan arsitektur (dari PDF)
- Pisahkan UI (Kotlin) dari mesin audio (C++). Audio callback TIDAK boleh
  melakukan disk I/O, logging berat, lock blocking, atau alokasi memori.
- Parameter dari UI ke DSP harus thread-safe (atomic parameter / command queue).
- Gunakan smoothing/ramping pada gain/mix agar tidak ada klik/pop.
- Versi awal: satu input aktif pada satu waktu; urutan efek tetap + bypass individual.
- Jangan janjikan angka latensi sebelum diukur pada perangkat nyata.

## Progres pembangunan (17 BAB di PDF)
- [x] BAB 0 — Toolchain + scaffold proyek (build sukses)
- [x] BAB 1 — Proyek Android Studio, package, Gradle, tema dasar
- [x] BAB 2 — Navigasi & kerangka layar (Home, Effects, Presets, Tuner, Settings)
- [x] BAB 3 — NDK, CMake, integrasi JNI (build native 3 ABI sukses)
- [x] BAB 4 — Oboe, buka input/output stream (full-duplex, siap iRig/USB)
- [x] BAB 5 — Pilih mikrofon internal & deteksi USB (daftar perangkat nyata + pemilihan)
- [x] BAB 6 — Audio callback & level meter (peak input/output, xrun, diagnostik)
- [x] BAB 7 — Noise Gate & Compressor (dinamika, bypass, smoothing per sampel)
- [x] BAB 8 — Overdrive, Distortion, Fuzz (saturasi ADAA, anti-aliasing)
- [x] BAB 9 — Equalizer (bass/mid/treble, biquad RBJ)
- [ ] BAB 10 — Chorus, Flanger, Phaser, Tremolo
- [ ] BAB 11 — Delay & Reverb
- [ ] BAB 12 — Pengelola urutan efek & parameter
- [ ] BAB 13 — Sistem preset
- [ ] BAB 14 — Tuner
- [ ] BAB 15 — Pengaturan audio, routing, lifecycle, error handling
- [ ] BAB 16 — Optimasi performa & uji perangkat
- [ ] BAB 17 — Build release & signing APK

## Konvensi kode
- Satu BAB diselesaikan & di-build sebelum lanjut BAB berikutnya.
- Komentar kode ditulis singkat dan hanya untuk hal yang tidak jelas dari kode.
- Struktur folder mengikuti PDF bagian 5 (usulan awal, boleh disesuaikan).
- Dokumen ini (`AGENTS.md`) diperbarui setiap menyelesaikan satu BAB.
