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
- [x] BAB 10 — Chorus, Flanger, Phaser, Tremolo (modulasi)
- [x] BAB 11 — Delay & Reverb (ruang/waktu)
- [x] BAB 12 — Pengelola urutan efek & parameter (order dapat diubah)
- [x] BAB 13 — Sistem preset (bawaan + pengguna)
- [x] BAB 14 — Tuner (deteksi pitch YIN, referensi A4)
- [x] BAB 15 — Pengaturan audio, routing, lifecycle, error handling
- [x] BAB 16 — Optimasi performa & pengukuran latensi
- [x] BAB 17 — Build release & signing APK (keystore aman)
- [x] Pasca-BAB 17 — Gap-fill blueprint: mute & bypass keseluruhan, perbaikan
      klik preset (smoothing EQ), preset aktif di Home, pemilih input/output
      cepat, strip pedalboard, pesan input kosong + saran headphone, panel izin,
      penanganan perangkat terputus, info versi nyata. Uji integrasi 12 efek.
- [x] Gap-fill lanjutan — izin & deteksi USB (iRig), uji stabilitas 60 detik,
      uji level rendah/tinggi, uji bypass tiap efek, ikon launcher final,
      ducking audio focus (perbaikan bug), pemilihan sample rate, restore default
      preset, reset seluruh efek saat interupsi (BAB 7), perbaikan bug indeks
      DelayLine (heap-buffer-overflow) yang ditemukan lewat AddressSanitizer.
- [x] Pasca-rilis — audit menyeluruh + perbaikan "APK tidak berfungsi":
      (a) perbaikan bug scroll bersarang di layar Tone (ScreenScaffold sudah
      scrollable; scroll ganda memicu crash pengukuran tinggi), (b) efek **Amp**
      baru (model amplifier: preamp ADAA -> tone stack bass/mid/treble ->
      presence -> kabinet low-pass -> master; voicing Clean/Crunch/Lead; id 5),
      (c) UI bergaya Tonebridge: komponen `Knob` putar + panel `AmpPanel` di Home
      dan knob pada tiap layar efek, (d) pustaka **Tone** (`ToneLibrary`, 26 tone
      bernama lagu/artis + pencarian) di layar Tone, (e) preset bawaan disusun
      ulang memakai Amp sebagai inti, (f) Amp aktif bawaan (sekali, dihargai bila
      pengguna mematikannya), (g) `DriveEffect`/gate/level sudah dipastikan benar
      lewat suite host. 13 suite DSP + ASAN lolos, JNI 59/59, debug/release/AAB
      build sukses, APK rilis tertanda tangan ALTOMEDIA.
- [x] Hardening pembukaan stream audio (perbaikan "APK tidak berbunyi"):
      strategi berlapis `openOutputStream`/`openInputStream` (Exclusive ->
      Shared -> None -> sample rate bawaan perangkat), `setPackageName` selalu
      diisi (wajib untuk input di Android 12+), format/kanal conversion
      diizinkan, DeviceId spesifik dibuang setelah percobaan pertama gagal,
      guard pointer null di callback, dan `onErrorAfterClose` memakai
      `try_lock` + memeriksa hasil restart. Ditambah dukungan iRig ke lubang
      headset: preset `VoiceRecognition` dipilih untuk input headset, pemilihan
      input headset/USB otomatis saat start & hotplug. Limiter & bypass global
      kini bertahan antar sesi; `resumeAfterInterruption` hanya menyalakan audio
      bila pengguna memang menekan Start.
- [x] Perbaikan kualitas suara & UI (masukan pengguna uji perangkat):
      1) **Bising** — Noise Gate kini aktif secara bawaan (diaktifkan di
         `MainActivity` lewat `EffectController`, karena kontrak uji bypass DSP
         mengharuskan efek default nonaktif di sisi C++). Ditambah waktu tahan
         (hold) internal 90 ms supaya gate tidak memotong nada yang meluruh,
         dan threshold bawaan dinaikkan ke -45 dB. Semua preset bawaan kini
         memuat Noise Gate.
      2) **Volume output kecil** — level output master bawaan +12 dB (sebelumnya
         0 dB), dengan migrasi pengaturan (`SETTINGS_VERSION`) agar instalasi
         lama ikut naik tanpa mengubah nilai yang pernah disetel pengguna.
         Rentang slider Level Output diperluas ke -40..+18 dB.
      3) **Efek terlalu singkat** — skala drive dinaikkan (Overdrive 30->200,
         Distortion 120->800, Fuzz 400->2000) sehingga sinyal tetap terjenuh
         selama nada meluruh (~3,5 s), bukan hanya ~1 s pada serangan awal.
      4) **UI pedal** — komponen `PedalBox` baru: kotak pedal dengan LED,
         tombol ON/OFF bergaya sakelar kaki (bukan tombol geser), dan tombol
         Preset cepat. Dipakai di layar Effects dan Home.
      5) **Pengaturan per efek** — rute `effect/{id}` + `EffectDetailScreen`;
         setiap efek punya halamannya sendiri (parameter, preset cepat, posisi
         rantai, reset), tidak lagi satu halaman gabungan.
- [x] Latar belakang aplikasi memakai gambar brand `Altomedia.png` (disalin ke
      `res/drawable-nodpi/bg_altomedia.png` agar tidak diskalakan densitas).
      Komponen `AppBackground` menggambar gambar penuh layar (crop) plus lapisan
      gelap (scrim) agar teks/panel tetap terbaca; `Scaffold` transparan dan
      bilah navigasi semi-transparan. Panel (`PanelCard`) dibuat agak
      transparan agar gambar tetap terlihat.

## Konvensi kode
- Satu BAB diselesaikan & di-build sebelum lanjut BAB berikutnya.
- Komentar kode ditulis singkat dan hanya untuk hal yang tidak jelas dari kode.
- Struktur folder mengikuti PDF bagian 5 (usulan awal, boleh disesuaikan).
- Dokumen ini (`AGENTS.md`) diperbarui setiap menyelesaikan satu BAB.

## Perintah kerja
- Build debug: `source ~/.android_env.sh && ./gradlew assembleDebug`
- Build release (bertanda tangan): `./gradlew assembleRelease` (kredensial dari `keystore.properties`, lihat `RELEASE.md`)
- Uji DSP di host (tanpa perangkat): `bash app/src/main/cpp/tests/run_tests.sh`
  - Cakupan: dsp, drive, eq, mod, space, order, pitch, gain, perf, level,
    bypass, catalog, integration.
  - `integration_test`: 12 efek aktif sekaligus, blok 128 frame, bypass dry
    identik, mute halus, perubahan parameter & urutan saat berjalan, serta
    stabilitas 60 detik.
  - `level_test`: jalur master nyata (trim input -> efek -> level output ->
    limiter) pada level rendah/tinggi/diam.
  - `bypass_test`: tiap efek mengubah suara saat aktif dan identik bit-per-bit
    saat bypass; plus kombinasi dua efek.
  - `space_test`: ditambah regresi `DelayLine` (indeks tidak keluar batas).
  - `catalog_test`: kontrak katalog BAB 7 — nama & jumlah parameter tiap efek
    sesuai blueprint, set/get parameter, setiap efek mengubah sinyal, reset
    menyetel ulang keadaan, stabil pada 96 kHz, dan `EffectType.kt` sinkron
    dengan DSP C++ (id/label/jumlah parameter). Jalankan suite ini setelah
    menambah/mengubah efek atau parameter.
- Build artefak rilis: `./gradlew assembleDebug assembleRelease bundleRelease`
  lalu `bash scripts/collect_artifacts.sh` (menyalin APK debug/release + AAB ke
  `artifacts/` dan menulis `artifacts/SHA256SUMS.txt`). Artefak ini SENGAJA
  di-commit (lihat pengecualian di `.gitignore`).
- Verifikasi rilis (butuh perangkat/emulator): `bash scripts/verify_release.sh`
  (tidak tersedia di container ini karena tanpa KVM/emulator).
- Kontrak JNI: `bash scripts/check_jni.sh` memastikan setiap `external fun
  nativeXxx` di NativeBridge.kt punya implementasi C++ di native-lib.cpp (dan
  sebaliknya). Jalankan setelah menambah/menghapus fungsi native.
- Pemeriksaan memori DSP: `ASAN=1 bash app/src/main/cpp/tests/run_tests.sh`
  menjalankan seluruh suite dengan AddressSanitizer + UndefinedBehaviorSanitizer.
  Selalu jalankan ini setelah mengubah kode buffer/index DSP.
- Pengaturan yang bertahan antar sesi disimpan di SharedPreferences
  (`alto_audio`) dan dimuat lewat `AudioController.init(context)` (dipanggil dari
  MainActivity.onCreate): perangkat input/output, sample rate, referensi tuner
  A4, trim input, gain output, dan mute. Jangan menghapus panggilan init; tanpa
  itu pengaturan tidak bertahan dan mute bisa tampak tidak konsisten.
- Mute master disimpan juga, jadi bila pengguna keluar saat mute aktif, UI
  menampilkan Mute: ON pada sesi berikutnya (tombol diberi warna error).
- Latensi terukur (BAB 16): `AudioEngine::measuredLatencyMs` memakai timestamp
  Oboe (`getTimestamp`) + `LatencyMath.h::roundTripLatencyMs`; nilai negatif
  berarti perangkat tidak melaporkan timestamp yang dapat dipercaya. Panel
  Performa menampilkan nilai terukur dan estimasi secara terpisah, dan
  menegaskan estimasi bukan hasil pengukuran (blueprint bagian 9).
- JANGAN memakai float untuk menghitung indeks buffer delay yang besar
  (`DelayLine::read`): pembulatan float dapat menghasilkan indeks == size_.
  Gunakan double dan jaga batas indeks.
- Verifikasi tanda tangan APK: `apksigner verify --print-certs app/build/outputs/apk/release/app-release.apk`

## Catatan penting
- Efek baru: daftarkan di `EffectChain` (`Effects.cpp`), tambahkan JNI bila perlu, lalu
  tambahkan entri di `EffectType.kt` (ID harus sama dengan enum C++).
- Urutan efek disimpan sebagai kode 4-bit per slot dalam satu atomik 64-bit; jangan
  mengubah urutan enum tanpa memperbarui kedua sisi.
- Parameter DSP diubah dari UI lewat atomik; jangan menyentuh objek DSP dari dua thread.
- Preset disimpan sebagai JSON di SharedPreferences (`PresetRepository`).
- `keystore.properties` dan `*.jks` tidak boleh di-commit. APK/AAB di
  `artifacts/` justru WAJIB di-commit setiap kali build (permintaan proyek).
- README.md adalah dokumentasi publik proyek; perbarui bila fitur/perintah
  berubah, dan commit bersama artefak.
- Perubahan parameter yang bisa mendadak (mis. memuat preset) WAJIB dihaluskan
  per sampel lewat `SmoothedParam`; uji `eq_test` memuat kasus 'Preset jump'.
- Mute master ada di `GainStage` (target efektif 0 dengan perataan); bypass
  keseluruhan ada di `EffectChain` (meneruskan sinyal dry apa adanya).
- Perangkat aktif yang dicabut ditangani di `MainActivity` (AudioDeviceCallback)
  lewat `AudioController.handleInputDisconnected/handleOutputDisconnected`.
- Audio focus: `LOSS_TRANSIENT_CAN_DUCK` -> `Event.DUCK` (turun -12 dB lewat
  `duckGain_` di engine, berperataan), `GAIN` -> `Event.UNDUCK` lalu `Event.GAIN`.
  Jangan memetakan duck ke resume/pause.
- USB Audio Interface: izin diminta lewat `UsbAudioPermission` +
  `res/xml/usb_device_filter.xml` (kelas audio = 1). Jangan menambah
  `android.permission.USB_PERMISSION` (bukan izin Android yang nyata).
- Sample rate: `AudioEngine::setPreferredSampleRate` (default 48000). Perangkat
  dapat mengabaikannya; `sampleRate()` selalu melaporkan rate stream sebenarnya.
- Efek **Amp** (`AmpModel`, id 5) adalah inti suara bergaya Tonebridge. Parameternya:
  0 voicing (0 Clean / 1 Crunch / 2 Lead, dibulatkan), 1 gain dB, 2 bass, 3 mid,
  4 treble, 5 presence, 6 master. Urutan pemrosesan internal: high-pass input ->
  preamp ADAA (saturasi) -> tone stack (bass low-shelf, mid peak, treble/presence
  high-shelf) -> low-pass kabinet -> DC blocker -> master. Semua dihaluskan.
- Amp diaktifkan sekali pada instalasi baru lewat `AudioController.applyAmpDefault`
  (kunci `amp_default_done`); bila pengguna mematikannya, pilihan itu dihormati.
- `ToneLibrary` (Kotlin) berisi tone terkurasi (nama/artis/gaya -> preset); bukan
  sampel audio asli, hanya setelan parameter. Jangan menambahkan klaim bahwa tone
  adalah rekaman lagu.
- Jangan menambahkan scroll bersarang: `ScreenScaffold` sudah `verticalScroll`.
  Menaruh `LazyColumn`/`verticalScroll` di dalamnya memicu crash pengukuran tinggi.
- Indeks slot pada `order_test.cpp` bergantung pada urutan enum; bila menambah efek,
  perbarui posisi yang diuji (test sudah memakai indeks enum Equalizer).
- Pembukaan stream HARUS berlapis (fallback): jangan memaksa Exclusive +
  Unprocessed + 1 kanal + 48 kHz tanpa alternatif. Banyak perangkat menolak
  kombinasi itu sehingga aplikasi tampak "tidak berbunyi". `setPackageName`
  wajib diisi (input Android 12+). Setelah percobaan pertama gagal, buang
  DeviceId spesifik agar memakai perangkat default sistem.
- iRig ke lubang headset = perangkat "Headset kabel" (TYPE_WIRED_HEADSET).
  Untuk input headset, coba `InputPreset::VoiceRecognition` lebih dulu
  (membuka mikrofon TRRS), lalu `Generic`. Sinyal gitar tetap diutamakan lewat
  `Unprocessed` untuk mikrofon internal/USB.
