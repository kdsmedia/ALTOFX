cmake_minimum_required(VERSION 3.22.1)
project(altoguitarfx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Oboe disediakan oleh dependensi Gradle lewat prefab (BAB 4).
find_package(oboe REQUIRED CONFIG)

add_library(altoguitarfx SHARED
        native-lib.cpp
        AudioEngine.cpp
        Effects.cpp
        NoiseGate.cpp
        Compressor.cpp
        DriveEffect.cpp
        Equalizer.cpp
)

find_library(log-lib log)
target_link_libraries(altoguitarfx ${log-lib} oboe::oboe)
