# ALTO GUITAR FX — Catatan Rilis (BAB 17)

Aplikasi multi-efek gitar real-time untuk Android. Pemrosesan DSP ditulis dalam
C++ (NDK) dan antarmuka memakai Jetpack Compose.

- Nama aplikasi: **ALTO GUITAR FX**
- Package: `com.altomedia.altoguitarfx`
- Versi: `1.0` (versionCode 1)
- minSdk 26 (Android 8.0), targetSdk 34
- ABI: `arm64-v8a`, `armeabi-v7a`, `x86_64`

## Membangun APK release

1. Siapkan keystore release:

   ```
   keytool -genkeypair -v -keystore altoguitarfx-release.jks \
     -alias altoguitarfx -keyalg RSA -keysize 2048 -validity 10000 \
     -dname "CN=ALTO GUITAR FX, O=ALTOMEDIA, C=ID"
   ```

2. Salin `keystore.properties.example` menjadi `keystore.properties` dan isi
   kredensial. Berkas ini dan berkas `.jks` **tidak** masuk version control.

3. Build:

   ```
   ./gradlew assembleRelease
   ```

   Hasil: `app/build/outputs/apk/release/app-release.apk`.

Bila `keystore.properties` atau keystore tidak ada, build release memakai debug
signing agar APK tetap dapat dibangun dan diuji.

## Verifikasi tanda tangan

```
apksigner verify --print-certs app/build/outputs/apk/release/app-release.apk
```

## Checklist sebelum rilis

- [x] Nama aplikasi, ikon, versi, dan package name final.
- [x] Izin mikrofon (`RECORD_AUDIO`) dan USB host dijelaskan di Settings.
- [x] Alur start/stop audio dan pergantian perangkat ditangani tanpa crash.
- [x] Setiap tombol/knob memiliki fungsi nyata yang terhubung ke DSP.
- [x] Audio callback bebas alokasi, lock blocking, dan I/O.
- [x] Preset bawaan dan penyimpanan preset diuji.
- [x] Build release ditandatangani dengan keystore aman.
- [ ] Uji instalasi APK release pada perangkat Android target. Jalankan
      `bash scripts/verify_release.sh` pada mesin dengan perangkat/emulator
      terhubung untuk memverifikasi tanda tangan, label, instalasi, dan
      ketiadaan crash saat buka/tutup.
- [x] Latensi round-trip diukur dari timestamp Oboe (bukan angka teoretis).
- [x] Kontrak JNI sinkron (`bash scripts/check_jni.sh`): 57 fungsi native
      terdeklarasi di Kotlin dan terimplementasi di C++.
- [x] Artefak APK debug/release + AAB beserta checksum di-commit di `artifacts/`.

## Artefak rilis di repositori

Setiap build rilis meng-commit artefak ke folder `artifacts/` agar dapat
diverifikasi ulang dari repositori:

```bash
./gradlew assembleDebug assembleRelease bundleRelease
bash scripts/collect_artifacts.sh
```

- `artifacts/altoguitarfx-debug.apk` — APK debug (uji internal).
- `artifacts/altoguitarfx-release.apk` — APK release, ditandatangani keystore release.
- `artifacts/altoguitarfx-release.aab` — Android App Bundle untuk distribusi Play.
- `artifacts/SHA256SUMS.txt` — checksum SHA-256 ketiga berkas di atas.

## Kompatibilitas interface audio

Perangkat diuji memakai jalur Oboe low-latency dengan format float dan input
`Unprocessed` (tanpa AGC/noise-suppression) agar karakter gitar tetap utuh.

- Input didukung: mikrofon internal, headset kabel, USB audio interface
  (mis. iRig), USB headset.
- Sumber dipilih di **Settings → Sumber Input**; daftar perangkat berasal dari
  sistem dan diperbarui otomatis saat dicabut/dicolok.
- Perubahan parameter dan perangkat dapat memicu penyambungan ulang stream
  otomatis (lihat pesan kesalahan di Settings).

## Rekomendasi

- Gunakan **headphone** untuk mencegah feedback, terutama saat memakai mikrofon
  internal.
- Atur **Trim Input** agar indikator clipping input tidak menyala.
- Jaga **Beban puncak** callback di bawah 80% (panel Performa & Latensi) untuk
  menghindari drop-out.
- Aktifkan **Batas Limiter** (default 0 dBFS) untuk mencegah clipping keras di
  perangkat output.
