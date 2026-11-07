# ALTO GUITAR FX

Aplikasi multi-efek gitar **real-time** untuk Android. Pemrosesan DSP ditulis
dalam C++ melalui Android NDK (Oboe/AAudio, jalur low-latency), antarmuka memakai
Jetpack Compose dengan tema gelap profesional.

Seluruh implementasi mengikuti blueprint `ALTO_GUITAR_FX_Rancangan_Lengkap.pdf`
(BAB 1–17). Tidak ada data dummy atau simulasi: setiap kontrol UI terhubung ke
mesin DSP nyata, dan daftar perangkat berasal dari sistem Android.

## Identitas aplikasi

| Aspek | Nilai |
| --- | --- |
| Nama aplikasi | ALTO GUITAR FX |
| Package | `com.altomedia.altoguitarfx` |
| Versi | 1.0 (versionCode 1) |
| minSdk / targetSdk | 26 (Android 8.0) / 34 |
| ABI | `arm64-v8a`, `armeabi-v7a`, `x86_64` |
| Format DSP internal | float 32-bit |
| Sample rate default | 48 kHz (dapat diubah bila perangkat mendukung) |

## Fitur

- **12 efek** (blueprint bagian 7): Noise Gate, Compressor, Overdrive,
  Distortion, Fuzz, Equalizer, Chorus, Flanger, Phaser, Tremolo, Delay, Reverb.
  Setiap efek punya parameter, bypass, dan reset sendiri.
- **Urutan efek** dapat diubah dari UI (naik/turun, reset ke bawaan).
- **Preset**: 9 preset bawaan (CLEAN, BLUES, ACOUSTIC, CRUNCH, ROCK, HARD ROCK,
  METAL, LEAD SOLO, AMBIENT) + simpan, rename, duplikasi, hapus, dan restore
  default.
- **Tuner** dengan referensi A4 yang dapat diubah (432–445 Hz).
- **Level meter** input/output, indikator clipping, mute, dan bypass keseluruhan.
- **Perangkat**: pemilihan input/output (mikrofon internal, headset, USB audio
  seperti iRig), izin USB, deteksi cabut/colok otomatis. iRig yang dicolok ke
  lubang headset (jack 3.5 mm) muncul sebagai "Headset kabel" dan dipilih
  otomatis bila pengguna masih memakai input default.
- **Audio focus**: ducking (−12 dB) saat fokus sementara hilang, pause/resume
  otomatis pada lifecycle.
- **Performa & latensi**: beban callback rata-rata/puncak, jumlah xrun
  (drop-out), dan latensi round-trip terukur dari timestamp Oboe.
- **Persistensi**: pilihan perangkat, sample rate, referensi tuner, trim input,
  gain output, dan mute bertahan antar sesi.

## Struktur proyek

```
app/src/main/
├── AndroidManifest.xml
├── java/com/altomedia/altoguitarfx/
│   ├── MainActivity.kt            # titik masuk, lifecycle, audio focus
│   ├── navigation/                # AppNavigation, Destination
│   ├── ui/                        # theme, home, effects, presets, tuner, settings
│   ├── audio/                     # NativeBridge, AudioController, AudioDeviceManager,
│   │                              # AudioFocusController, UsbAudioPermission
│   ├── effects/                   # EffectType (katalog), EffectController
│   └── data/                      # Preset, PresetRepository, BuiltInPresets
├── cpp/                           # AudioEngine, Effects, seluruh DSP efek, JNI
│   └── tests/                     # 13 suite uji host (tanpa perangkat)
└── res/                           # tema, ikon launcher, filter perangkat USB
```

## Membangun

```bash
source ~/.android_env.sh          # SDK/NDK/CMake untuk lingkungan ini
./gradlew assembleDebug assembleRelease bundleRelease
```

Keluaran build:

- `app/build/outputs/apk/debug/app-debug.apk`
- `app/build/outputs/apk/release/app-release.apk`
- `app/build/outputs/bundle/release/app-release.aab`

### Artefak yang di-commit

Folder `artifacts/` menyimpan APK/AAB hasil build terakhir beserta checksum agar
dapat diverifikasi ulang dari repositori:

```bash
bash scripts/collect_artifacts.sh
```

- `artifacts/altoguitarfx-debug.apk`
- `artifacts/altoguitarfx-release.apk`
- `artifacts/altoguitarfx-release.aab`
- `artifacts/SHA256SUMS.txt`

Catatan keamanan: kredensial signing (`keystore.properties`, `*.jks`) **tidak**
masuk version control. APK release ditandatangani dengan keystore release nyata
(lihat `RELEASE.md`).

## Pengujian

```bash
# 13 suite DSP di host (tanpa perangkat)
bash app/src/main/cpp/tests/run_tests.sh

# Ditambah AddressSanitizer + UndefinedBehaviorSanitizer
ASAN=1 bash app/src/main/cpp/tests/run_tests.sh

# Kontrak JNI: deklarasi Kotlin vs implementasi C++ harus sinkron
bash scripts/check_jni.sh

# Verifikasi rilis (butuh perangkat/emulator lewat adb)
bash scripts/verify_release.sh
```

Suite host: `dsp`, `drive`, `eq`, `mod`, `space`, `order`, `pitch`, `gain`,
`perf`, `level`, `bypass`, `catalog`, `integration`. `catalog_test` juga
memverifikasi bahwa katalog UI (`EffectType.kt`) tetap sinkron dengan DSP C++.

Pengujian yang membutuhkan perangkat nyata (blueprint bagian 12): mikrofon
internal, USB audio interface (iRig) termasuk proses izin USB, headphone kabel,
cabut/pasang kabel USB, pindah background/foreground, pengukuran round-trip
latency, dan uji stabilitas jangka panjang.

## Izin

- `RECORD_AUDIO` — menangkap sinyal gitar dari mikrofon internal atau audio
  interface USB; diminta saat runtime.
- USB host — dideklarasikan dengan intent-filter `USB_DEVICE_ATTACHED` dan
  `res/xml/usb_device_filter.xml` (kelas audio) untuk mendeteksi iRig.

## Rekomendasi pemakaian

- Gunakan **headphone** untuk mencegah feedback, terutama dengan mikrofon
  internal.
- Atur **Trim Input** agar indikator clipping input tidak menyala.
- Jaga beban puncak callback di bawah 80% (panel Performa & Latensi).
- Aktifkan **Batas Limiter** (default 0 dBFS) untuk mencegah clipping keras.

## Dokumentasi terkait

- `RELEASE.md` — catatan rilis, signing, dan checklist sebelum rilis.
- `AGENTS.md` — catatan teknis internal dan konvensi pengembangan.
