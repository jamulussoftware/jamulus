cmake_minimum_required(VERSION 3.16)
project(jamulus_engine_plugin LANGUAGES CXX)

if (NOT COMMAND juce_add_plugin)
    # JUCE not available as CMake functions — try to find the installed/configured
    # JUCE package (e.g. from CMAKE_PREFIX_PATH). This path can point at a JUCE
    # build tree where JUCEConfig.cmake was generated.
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/third_party/JUCE" "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/JUCE")
    find_package(JUCE 6 CONFIG REQUIRED)
else()
    # JUCE was added via add_subdirectory earlier; the juce_add_plugin function
    # is available and we can proceed without find_package.
endif()

# Allow building additional plugin formats via cache options. AU is macOS-only.
option(ENABLE_VST2 "Enable VST2 format (requires VST2 SDK)" OFF)
option(ENABLE_AU "Enable AU format (macOS only)" OFF)
option(ENABLE_LV2 "Enable LV2 format" OFF)
option(ENABLE_CLAP "Enable CLAP format" OFF)
option(ENABLE_STANDALONE "Enable Standalone format" ON)

set(PLUGIN_FORMATS VST3)
if (ENABLE_VST2)
    list(APPEND PLUGIN_FORMATS VST2)
endif()
if (ENABLE_LV2)
    list(APPEND PLUGIN_FORMATS LV2)
endif()
if (ENABLE_AU AND APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()
if (ENABLE_STANDALONE)
    list(APPEND PLUGIN_FORMATS Standalone)
endif()

# ==============================================================================
# 1. THE CORE ENGINE (Renamed from jamulus_vst3 to jamulus_engine)
# ==============================================================================
# Use juce_add_plugin but we will force it to be a plain DLL
# Target name is jamulus_engine to avoid conflict with libjamulus's "jamulus_core"
juce_add_plugin(jamulus_engine
    PRODUCT_NAME "JamulusPlus"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/resources/JamulusPlusIcon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/resources/JamulusPlusIcon.png"
    VERSION 0.1
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    LV2URI "urn:jamulus:plugin"
    FORMATS ${PLUGIN_FORMATS}
)

target_sources(jamulus_engine PRIVATE
    PluginProcessor.cpp
    PluginProcessor.h
    PluginEditor.cpp
    PluginEditor.h
    MinimalCoreGui.h
    JamulusClientCompat.cpp
)

# Add the JUCE adapter which implements `createPluginFilter()` expected
# by JUCE's plugin client code.
target_sources(jamulus_engine PRIVATE
    JucePluginAdapter.cpp
    JucePluginAdapter.h
)

# Force Dynamic Runtime (MD) for consistent CRT usage across modules
set_target_properties(jamulus_engine PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

# Include the local ASIO shim in the plugin shared-code
# Real ASIO support comes from JUCE on Windows; avoid the local stub shim.

## Link against JUCE audio processors target.
if (TARGET juce::juce_audio_processors)
    target_link_libraries(jamulus_engine PRIVATE jamulus_core_api juce::juce_audio_processors)
    if (TARGET juce::juce_audio_devices)
        target_link_libraries(jamulus_engine PRIVATE juce::juce_audio_devices)
    endif()
    if (TARGET juce::juce_audio_utils)
        target_link_libraries(jamulus_engine PRIVATE juce::juce_audio_utils)
    endif()
    if (TARGET juce::juce_audio_formats)
        target_link_libraries(jamulus_engine PRIVATE juce::juce_audio_formats)
    endif()
    if (TARGET juce::juce_osc)
        target_link_libraries(jamulus_engine PRIVATE juce::juce_osc)
    endif()
elseif (TARGET JUCE::juce_audio_processors)
    target_link_libraries(jamulus_engine PRIVATE jamulus_core_api JUCE::juce_audio_processors)
    if (TARGET JUCE::juce_audio_devices)
        target_link_libraries(jamulus_engine PRIVATE JUCE::juce_audio_devices)
    endif()
    if (TARGET JUCE::juce_audio_utils)
        target_link_libraries(jamulus_engine PRIVATE JUCE::juce_audio_utils)
    endif()
    if (TARGET JUCE::juce_audio_formats)
        target_link_libraries(jamulus_engine PRIVATE JUCE::juce_audio_formats)
    endif()
    if (TARGET JUCE::juce_osc)
        target_link_libraries(jamulus_engine PRIVATE JUCE::juce_osc)
    endif()
else()
    message(FATAL_ERROR "Could not find a JUCE audio_processors target.")
endif()


# Prevent JUCE from erroring when building only the VST3 client
target_compile_definitions(jamulus_engine PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DIRECT2D=1)
if (WIN32)
    target_compile_definitions(jamulus_engine PRIVATE JUCE_ASIO=1 JAMULUS_USE_REAL_ASIO=1)
endif()

if (ENABLE_CLAP)
    if (NOT COMMAND clap_juce_extensions_plugin)
        message(FATAL_ERROR "ENABLE_CLAP=ON requires third_party/clap-juce-extensions to be available.")
    endif()

    clap_juce_extensions_plugin(
        TARGET jamulus_engine
        CLAP_ID "org.jamulussoftware.jamulusplus"
        CLAP_FEATURES audio-effect utility
        CLAP_MANUAL_URL "https://github.com/AndyMcProducer/jamulus"
        CLAP_SUPPORT_URL "https://github.com/AndyMcProducer/jamulus/issues"
    )
endif()

# Ensure the platform-specific plugin artefact targets link the wrapper too.
if (TARGET jamulus_engine_VST3)
    target_link_libraries(jamulus_engine_VST3 PRIVATE jamulus_core_api)
endif()

# Ensure platform-specific plugin artefact targets link the wrapper too.
foreach(fmt IN LISTS PLUGIN_FORMATS)
    string(TOUPPER ${fmt} FMT_UPPER)
    set(TGT_NAME "jamulus_engine_${FMT_UPPER}")
    if (TARGET ${TGT_NAME})
        target_link_libraries(${TGT_NAME} PRIVATE jamulus_core_api)
    endif()
    set(TGT_NAME_ALT "jamulus_engine_${fmt}")
    if (TARGET ${TGT_NAME_ALT})
        target_link_libraries(${TGT_NAME_ALT} PRIVATE jamulus_core_api)
    endif()
endforeach()

target_include_directories(jamulus_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (TARGET jamulus_engine_LV2)
    add_custom_command(TARGET jamulus_engine_LV2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Running LV2 helper wrapper to validate plugin load"
        COMMAND cmd /c "\"${CMAKE_SOURCE_DIR}\\build_tools\\run_lv2_helper_wrapper.bat\" \"$<TARGET_FILE_DIR:jamulus_engine_LV2>\" \"${CMAKE_BINARY_DIR}\\plugin\\plugin\\Release\\juce_lv2_helper.exe\""
        COMMENT "Validate LV2 plugin can be loaded by running helper with LV2 dir on PATH"
        VERBATIM
    )
endif()

# ==============================================================================
# 2. POST-BUILD DEPLOYMENT
# ==============================================================================
set(_jamulus_resources_dir "${CMAKE_CURRENT_SOURCE_DIR}/resources")

if (TARGET jamulus_engine_VST3)
    set(_engine_bundle_dir "${CMAKE_BINARY_DIR}/plugin/jamulus_engine_artefacts/$<CONFIG>/VST3/JamulusPlus.vst3")
    set(DIST_DIR "${CMAKE_BINARY_DIR}/../dist/JamulusPlus.vst3")
    set(JW_DIST_DIR "${CMAKE_SOURCE_DIR}/dist/JamulusPlus.vst3")
    if (EXISTS "${_jamulus_resources_dir}")
        add_custom_command(TARGET jamulus_engine_VST3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_engine_bundle_dir}/Contents/Resources"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${_jamulus_resources_dir}" "${_engine_bundle_dir}/Contents/Resources"
            COMMENT "Copying plugin theme resources into VST3 bundle"
            VERBATIM
        )
    endif()
    add_custom_target(jamulus_dist
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}/Contents/x86_64-win"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${_engine_bundle_dir}" "${DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${JW_DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${JW_DIST_DIR}/Contents/x86_64-win"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${_engine_bundle_dir}" "${JW_DIST_DIR}"
        COMMENT "Copying VST3 bundle to dist directories"
        VERBATIM
    )
    add_dependencies(jamulus_dist jamulus_engine_VST3)
endif()

if (TARGET jamulus_engine_Standalone)
    if (WIN32)
        set(_standalone_runtime_dir "${CMAKE_BINARY_DIR}/plugin/jamulus_engine_artefacts/$<CONFIG>/Standalone")
        set(_standalone_resources_dir "${_standalone_runtime_dir}/Resources")
        set(_standalone_dist_dir "${CMAKE_BINARY_DIR}/../dist/JamulusPlus-Standalone")
        set(_standalone_source_dist_dir "${CMAKE_SOURCE_DIR}/dist/JamulusPlus-Standalone")

        if (EXISTS "${_jamulus_resources_dir}")
            add_custom_command(TARGET jamulus_engine_Standalone POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${_standalone_resources_dir}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${_jamulus_resources_dir}" "${_standalone_resources_dir}"
                COMMENT "Copying standalone theme resources"
                VERBATIM
            )
        endif()

        add_custom_target(jamulus_dist_standalone
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${_standalone_dist_dir}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_standalone_dist_dir}/Resources"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:jamulus_engine_Standalone>" "${_standalone_dist_dir}/"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${_standalone_resources_dir}" "${_standalone_dist_dir}/Resources"
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${_standalone_source_dist_dir}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_standalone_source_dist_dir}/Resources"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:jamulus_engine_Standalone>" "${_standalone_source_dist_dir}/"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${_standalone_resources_dir}" "${_standalone_source_dist_dir}/Resources"
            COMMENT "Copying standalone app and resources to dist directories"
            VERBATIM
        )
    elseif(APPLE)
        set(_standalone_bundle_dir "$<TARGET_BUNDLE_DIR:jamulus_engine_Standalone>")
        set(_standalone_bundle_resources_dir "${_standalone_bundle_dir}/Contents/Resources")
        set(_standalone_dist_dir "${CMAKE_BINARY_DIR}/../dist/JamulusPlus.app")
        set(_standalone_source_dist_dir "${CMAKE_SOURCE_DIR}/dist/JamulusPlus.app")

        if (EXISTS "${_jamulus_resources_dir}")
            add_custom_command(TARGET jamulus_engine_Standalone POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${_standalone_bundle_resources_dir}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${_jamulus_resources_dir}" "${_standalone_bundle_resources_dir}"
                COMMENT "Copying standalone theme resources into app bundle"
                VERBATIM
            )
        endif()

        add_custom_target(jamulus_dist_standalone
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${_standalone_dist_dir}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${_standalone_bundle_dir}" "${_standalone_dist_dir}"
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${_standalone_source_dist_dir}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${_standalone_bundle_dir}" "${_standalone_source_dist_dir}"
            COMMENT "Copying standalone app bundle to dist directories"
            VERBATIM
        )
    endif()

    if (TARGET jamulus_dist_standalone)
        add_dependencies(jamulus_dist_standalone jamulus_engine_Standalone)
    endif()
endif()
