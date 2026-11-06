#!/usr/bin/env bash
# Memverifikasi kontrak JNI (BAB 3/12): setiap fungsi `external fun nativeXxx`
# di NativeBridge.kt harus punya implementasi
# `Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeXxx` di native-lib.cpp
# dan sebaliknya. Mencegah UnsatisfiedLinkError saat runtime.
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
KT="$ROOT/app/src/main/java/com/altomedia/altoguitarfx/audio/NativeBridge.kt"
CPP="$ROOT/app/src/main/cpp/native-lib.cpp"
PREFIX="Java_com_altomedia_altoguitarfx_audio_NativeBridge_"

KT_FUNS="$(grep -oE "external fun native[A-Za-z0-9_]+" "$KT" | awk '{print $3}' | sort)"
CPP_FUNS="$(grep -oE "${PREFIX}[A-Za-z0-9_]+" "$CPP" | sed "s/${PREFIX}//" | sort)"

MISSING_IN_CPP="$(comm -23 <(echo "$KT_FUNS") <(echo "$CPP_FUNS"))"
MISSING_IN_KT="$(comm -13 <(echo "$KT_FUNS") <(echo "$CPP_FUNS"))"

KT_COUNT="$(echo "$KT_FUNS" | grep -c . || true)"
CPP_COUNT="$(echo "$CPP_FUNS" | grep -c . || true)"
echo "JNI: $KT_COUNT deklarasi Kotlin, $CPP_COUNT implementasi C++"

FAIL=0
if [ -n "$MISSING_IN_CPP" ]; then
    echo "Deklarasi Kotlin tanpa implementasi C++:"
    echo "$MISSING_IN_CPP" | sed 's/^/  /'
    FAIL=1
fi
if [ -n "$MISSING_IN_KT" ]; then
    echo "Implementasi C++ tanpa deklarasi Kotlin:"
    echo "$MISSING_IN_KT" | sed 's/^/  /'
    FAIL=1
fi

if [ "$FAIL" -eq 0 ]; then
    echo "Kontrak JNI sinkron -> OK"
else
    echo "Kontrak JNI TIDAK sinkron -> GAGAL"
fi
exit "$FAIL"
