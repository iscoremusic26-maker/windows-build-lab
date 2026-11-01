cmake_minimum_required(VERSION 3.25)
project(Post01PassiveObserver LANGUAGES C CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
 message(FATAL_ERROR "Windows x64 MSVC required")
endif()
# Dusklight v2.0.3 oficial (DUSK_SOURCE) + biblioteca de importacion del ZIP oficial de Windows (DUSK_GAME_EXE). Sin build del host.
add_subdirectory("${DUSK_SOURCE}/sdk" sdk EXCLUDE_FROM_ALL)
add_mod(post01_passive_observer FEATURES game SOURCES src/mod.cpp MOD_JSON mod.json)
# /Zc:inline- conserva los registros MOD_META_RECORD (patron DEVF CP01, probado con MSVC); /GL- evita LTO sobre la metadata.
target_compile_options(post01_passive_observer PRIVATE /utf-8 /EHsc /Zc:__cplusplus /Zc:inline- /GL-)
if(DEFINED POST01_SOURCE_COMMIT)
 target_compile_definitions(post01_passive_observer PRIVATE POST01_SOURCE_COMMIT="${POST01_SOURCE_COMMIT}")
endif()
target_link_options(post01_passive_observer PRIVATE "/MAP:${CMAKE_CURRENT_BINARY_DIR}/post01_passive_observer.map")
# Laboratorio de logica pura (sin SDK ni cabeceras del juego): solo para CI/local, nunca se empaqueta.
add_executable(post01_core_lab EXCLUDE_FROM_ALL tests/core_lab.cpp)
target_compile_features(post01_core_lab PRIVATE cxx_std_20)
target_compile_options(post01_core_lab PRIVATE /utf-8 /EHsc /W4)
