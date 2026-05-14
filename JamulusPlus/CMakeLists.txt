cmake_minimum_required(VERSION 3.22)
project(jamulus_core_build LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(JAMULUS_NONQT_CORE "Use NonQtJamulusClientCore implementation in the wrapper" ON)
set(JAMULUS_NONQT_CORE ON CACHE BOOL "Use NonQtJamulusClientCore implementation in the wrapper" FORCE)

if (MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

if (NOT DEFINED JAMULUS_HEADLESS)
    set(JAMULUS_HEADLESS ON CACHE BOOL "Force headless build for wrapper" FORCE)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../libs/opus/CMakeLists.txt" AND NOT TARGET opus)
    set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(OPUS_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
    set(OPUS_CUSTOM_MODES ON CACHE BOOL "" FORCE)
    set(OPUS_ENABLE_FLOAT_API ON CACHE BOOL "" FORCE)
    set(OPUS_FIXED_POINT OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libs/opus ${CMAKE_BINARY_DIR}/opus)
endif()

# Use dynamic VC++ runtime for plugin/shared library compatibility

add_library(jamulus_core_api STATIC
    jamulus_core_juce.cpp
    juce_net_abstraction.cpp
    ../src/buffer.cpp
)

# Force Dynamic Runtime (MD) for consistent CRT usage across modules
set_target_properties(jamulus_core_api PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)
if (MSVC)
    target_compile_options(jamulus_core_api PRIVATE /Zc:__cplusplus /permissive-)
endif()

target_sources(jamulus_core_api PRIVATE audio_fifo.cpp)
# Real ASIO support comes from JUCE on Windows; avoid the local stub shim.

target_include_directories(jamulus_core_api PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/opus/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/opus/celt
)
target_compile_definitions(jamulus_core_api PRIVATE JAMULUS_USE_JUCE_XML=1)
if (TARGET juce::juce_core)
    target_link_libraries(jamulus_core_api PRIVATE juce::juce_core)
    if (TARGET juce::juce_events)
        target_link_libraries(jamulus_core_api PRIVATE juce::juce_events)
    endif()
elseif (TARGET JUCE::juce_core)
    target_link_libraries(jamulus_core_api PRIVATE JUCE::juce_core)
    if (TARGET JUCE::juce_events)
        target_link_libraries(jamulus_core_api PRIVATE JUCE::juce_events)
    endif()
endif()
if (TARGET opus)
    target_link_libraries(jamulus_core_api PRIVATE opus)
    target_compile_definitions(jamulus_core_api PRIVATE JAMULUS_USE_REAL_OPUS=1)
elseif (TARGET Opus::opus)
    target_link_libraries(jamulus_core_api PRIVATE Opus::opus)
    target_compile_definitions(jamulus_core_api PRIVATE JAMULUS_USE_REAL_OPUS=1)
endif()
# Link against winmm for MIDI APIs used by the Jamulus core on Windows
if (WIN32)
    target_link_libraries(jamulus_core_api PRIVATE winmm ws2_32)
endif()

# We need GUI support now - remove HEADLESS define
target_compile_definitions(jamulus_core_api PRIVATE APP_VERSION=\"0.0.0\" NOMINMAX JAMULUS_USE_JUCE_NET=1 JAMULUS_NO_QT_TRANSLATIONS=1)
if (JAMULUS_NONQT_CORE)
    target_compile_definitions(jamulus_core_api PRIVATE JAMULUS_NONQT_CORE=1)
endif()
if (JAMULUS_HEADLESS)
    target_compile_definitions(jamulus_core_api PRIVATE HEADLESS)
endif()

set(JAMULUS_USE_JUCE_NET ON CACHE BOOL "Build Jamulus core for JUCE net wrapper" FORCE)

target_sources(jamulus_core_api PRIVATE stubs_linker.cpp)

# If JUCE is available, build the VST3 plugin scaffold in `plugin/`.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugin/CMakeLists.txt")
    message(STATUS "No JUCE plugin scaffold found in plugin/")
else()
    # If a local JUCE checkout exists under third_party, add it so the
    # JUCE CMake targets are available. JUCE requires a newer CMake, so
    # ensure this project requests at least 3.22 above.
    if (JAMULUS_HEADLESS OR TRUE)
        if (TARGET juce::juce_core)
            target_link_libraries(jamulus_core_api PRIVATE juce::juce_core)
        elseif (TARGET JUCE::juce_core)
            target_link_libraries(jamulus_core_api PRIVATE JUCE::juce_core)
        endif()
    endif()
    # Embed Jamulus assets (images, sounds) into a static library usable from jamulus_wrapper
    file(GLOB_RECURSE JAMULUS_ASSET_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../src/res/*")
    if (JAMULUS_ASSET_FILES)
        juce_add_binary_data(jamulus_assets SOURCES ${JAMULUS_ASSET_FILES})
        # Link assets publicly so final plugin targets also get the objects
        target_link_libraries(jamulus_core_api PUBLIC jamulus_assets)
    endif()
    add_subdirectory(plugin)

    # Probe tool to scan and instantiate a VST3 using JUCE's headless plugin-hosting code.
    # This prints a concrete error string when a plugin "fails to load".
    if (TARGET juce_audio_processors_headless AND NOT TARGET juce::juce_audio_processors_headless)
        add_library(juce::juce_audio_processors_headless ALIAS juce_audio_processors_headless)
    endif()

    if (TARGET juce::juce_audio_processors_headless)
        add_executable(vst3_load_probe
            test/vst3_load_probe.cpp
        )
        target_link_libraries(vst3_load_probe PRIVATE juce::juce_audio_processors_headless)
        target_compile_definitions(vst3_load_probe PRIVATE JUCE_PLUGINHOST_VST3=1)
        set_target_properties(vst3_load_probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    else()
        message(STATUS "JUCE headless hosting module not available; skipping vst3_load_probe")
    endif()
endif()

# If user chose to build the full standalone app (not headless), provide a
# native `jamulus` executable target that links the `jamulus_core` static
# library. This allows a single CMake configure to produce either the plugin
# wrapper or the standalone application depending on `JAMULUS_HEADLESS`.
if (NOT JAMULUS_HEADLESS AND TARGET jamulus_core)
    add_executable(jamulus_app
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/main.cpp
    )
    target_link_libraries(jamulus_app PRIVATE jamulus_core)
    target_include_directories(jamulus_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../src ${CMAKE_CURRENT_SOURCE_DIR}/..)
    target_compile_definitions(jamulus_app PRIVATE APP_VERSION=\"0.0.0\" NOMINMAX JAMULUS_USE_JUCE_XML=1)
    if (TARGET juce::juce_core)
        target_link_libraries(jamulus_app PRIVATE juce::juce_core)
        if (TARGET juce::juce_events)
            target_link_libraries(jamulus_app PRIVATE juce::juce_events)
        endif()
        if (TARGET juce::juce_gui_basics)
            target_link_libraries(jamulus_app PRIVATE juce::juce_gui_basics)
        endif()
    elseif (TARGET JUCE::juce_core)
        target_link_libraries(jamulus_app PRIVATE JUCE::juce_core)
        if (TARGET JUCE::juce_events)
            target_link_libraries(jamulus_app PRIVATE JUCE::juce_events)
        endif()
        if (TARGET JUCE::juce_gui_basics)
            target_link_libraries(jamulus_app PRIVATE JUCE::juce_gui_basics)
        endif()
    endif()
    set_target_properties(jamulus_app PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    message(STATUS "Added standalone 'jamulus_app' executable target (non-headless)")
endif()
