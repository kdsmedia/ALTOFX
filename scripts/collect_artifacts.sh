#!/usr/bin/env bash
# Mengumpulkan artefak rilis (APK debug/release + AAB) ke folder artifacts/
# lalu menulis checksum SHA-256. Dijalankan setelah build.
# Pemakaian: bash scripts/collect_artifacts.sh
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/artifacts"
mkdir -p "$OUT"

copy() {
    local src="$1" dst="$2"
    if [ -f "$src" ]; then
        cp -f "$src" "$dst"
        echo "disalin: ${dst#$ROOT/}"
    else
        echo "LEWATI (tidak ada): ${src#$ROOT/}"
    fi
}

copy "$ROOT/app/build/outputs/apk/debug/app-debug.apk" "$OUT/altoguitarfx-debug.apk"
copy "$ROOT/app/build/outputs/apk/release/app-release.apk" "$OUT/altoguitarfx-release.apk"
copy "$ROOT/app/build/outputs/bundle/release/app-release.aab" "$OUT/altoguitarfx-release.aab"

if command -v sha256sum >/dev/null 2>&1; then
    ( cd "$OUT" && sha256sum *.apk *.aab 2>/dev/null > SHA256SUMS.txt )
    echo "checksum ditulis: artifacts/SHA256SUMS.txt"
fi
