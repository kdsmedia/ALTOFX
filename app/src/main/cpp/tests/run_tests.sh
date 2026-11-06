#!/usr/bin/env bash
# Menjalankan seluruh uji DSP di host (tanpa perangkat Android).
# Pemakaian: bash app/src/main/cpp/tests/run_tests.sh
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
INC="$(dirname "$HERE")"
ALL="$INC/Effects.cpp $INC/NoiseGate.cpp $INC/Compressor.cpp $INC/DriveEffect.cpp $INC/Equalizer.cpp $INC/Chorus.cpp $INC/Flanger.cpp $INC/Phaser.cpp $INC/Tremolo.cpp $INC/Delay.cpp $INC/Reverb.cpp"
fail=0
for t in dsp_test drive_test eq_test mod_test space_test order_test pitch_test gain_test perf_test integration_test; do
  echo "===== $t ====="
  if g++ -std=c++17 -I"$INC" -o "/tmp/$t" "$HERE/$t.cpp" $ALL $INC/PitchDetector.cpp; then
    "/tmp/$t" || fail=1
  else
    echo "COMPILE FAIL"; fail=1
  fi
done
echo "FAIL=$fail"
exit $fail
