#!/usr/bin/env bash
# Verifikasi rilis ALTO GUITAR FX (checklist BAB 14).
# Membutuhkan perangkat/emulator yang terhubung lewat adb.
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APK="$ROOT/app/build/outputs/apk/release/app-release.apk"
SDK="${ANDROID_HOME:-$HOME/Android/Sdk}"
ADB="$SDK/platform-tools/adb"

echo "== 1. Tanda tangan APK =="
"$SDK/build-tools/34.0.0/apksigner" verify --print-certs "$APK" \
  | grep -E "Signer #1 certificate DN|SHA-256"

echo "== 2. Label, versi, package =="
"$SDK/build-tools/34.0.0/aapt2" dump badging "$APK" \
  | grep -E "^package:|^application-label:|^application:"

echo "== 3. Perangkat terhubung =="
if ! "$ADB" devices | grep -q "device$"; then
  echo "Tidak ada perangkat/emulator terhubung. Lewati langkah instalasi."
  exit 0
fi

echo "== 4. Instal ulang APK release =="
"$ADB" install -r "$APK"

echo "== 5. Uji buka/tutup (tidak boleh crash) =="
"$ADB" shell am start -n com.altomedia.altoguitarfx/.MainActivity >/dev/null
sleep 3
"$ADB" shell am force-stop com.altomedia.altoguitarfx
sleep 1
if "$ADB" logcat -d -b crash | grep -q "com.altomedia.altoguitarfx"; then
  echo "Ditemukan crash pada logcat:"
  "$ADB" logcat -d -b crash | tail -30
  exit 1
fi
echo "Tidak ada crash terdeteksi."
