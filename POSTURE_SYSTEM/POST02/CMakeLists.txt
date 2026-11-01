cmake_minimum_required(VERSION 3.25)
project(Post02InstanceLifecycle LANGUAGES C CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
 message(FATAL_ERROR "Windows x64 MSVC required")
endif()
# Dusklight v2.0.3 oficial (DUSK_SOURCE) + biblioteca de importacion del ZIP oficial de Windows (DUSK_GAME_EXE). Sin build del host.
add_subdirectory("${DUSK_SOURCE}/sdk" sdk EXCLUDE_FROM_ALL)
add_mod(post02_instance_lifecycle FEATURES game SOURCES src/mod.cpp MOD_JSON mod.json)
# /Zc:inline- conserva los registros MOD_META_RECORD (patron DEVF CP01 / POST01, probado con MSVC); /GL- evita LTO sobre la metadata.
target_compile_options(post02_instance_lifecycle PRIVATE /utf-8 /EHsc /Zc:__cplusplus /Zc:inline- /GL-)
if(DEFINED POST02_SOURCE_COMMIT)
 target_compile_definitions(post02_instance_lifecycle PRIVATE POST02_SOURCE_COMMIT="${POST02_SOURCE_COMMIT}")
endif()
target_link_options(post02_instance_lifecycle PRIVATE "/MAP:${CMAKE_CURRENT_BINARY_DIR}/post02_instance_lifecycle.map")
# Laboratorio de logica pura (sin SDK ni cabeceras del juego): solo CI/local, nunca se empaqueta.
add_executable(post02_lifecycle_lab EXCLUDE_FROM_ALL tests/lifecycle_lab.cpp)
target_compile_features(post02_lifecycle_lab PRIVATE cxx_std_20)
target_compile_options(post02_lifecycle_lab PRIVATE /utf-8 /EHsc /W4)
