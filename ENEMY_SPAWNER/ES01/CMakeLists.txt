cmake_minimum_required(VERSION 3.25)
project(EnemySpawnerStandalone LANGUAGES C CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
 message(FATAL_ERROR "Windows x64 MSVC required")
endif()
# Dusklight 2.0.3 official sources (DUSK_SOURCE) + the import library of the official Windows ZIP (DUSK_GAME_EXE). The host is never built.
add_subdirectory("${DUSK_SOURCE}/sdk" sdk EXCLUDE_FROM_ALL)
add_mod(enemy_spawner_standalone FEATURES game SOURCES src/mod.cpp MOD_JSON mod.json)
# /Zc:inline- keeps the MOD_META records; /GL- keeps link-time code generation off the metadata.
target_compile_options(enemy_spawner_standalone PRIVATE /utf-8 /EHsc /Zc:__cplusplus /Zc:inline- /GL- /W4)
target_link_options(enemy_spawner_standalone PRIVATE "/MAP:${CMAKE_CURRENT_BINARY_DIR}/enemy_spawner_standalone.map")
# Pure laboratories (no SDK, no game headers): CI and local only, never packaged.
foreach(lab core_lab adapter_lab)
 add_executable(es_${lab} EXCLUDE_FROM_ALL tests/${lab}.cpp)
 target_compile_features(es_${lab} PRIVATE cxx_std_20)
 target_compile_options(es_${lab} PRIVATE /utf-8 /EHsc /W4)
endforeach()
