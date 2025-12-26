cmake_minimum_required(VERSION 3.16)
project(jamulus_vst3 LANGUAGES CXX)

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

juce_add_plugin(jamulus_vst3
    PRODUCT_NAME "JamulusVST3"
    VERSION 0.1
    IS_SYNTH FALSE
    NEEDS_MIDI FALSE
    FORMATS VST3
)

target_sources(jamulus_vst3 PRIVATE
    PluginProcessor.cpp
    PluginProcessor.h
    PluginEditor.cpp
    PluginEditor.h
)

# Add the JUCE adapter which implements `createPluginFilter()` expected
# by JUCE's plugin client code.
target_sources(jamulus_vst3 PRIVATE
    JucePluginAdapter.cpp
    JucePluginAdapter.h
)

# Include the local ASIO shim in the plugin shared-code so final plugin
# artefacts get the ASIO stub symbols (e.g. ASIOControlPanel) available
# at link time.
target_sources(jamulus_vst3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../asioshim.cpp)

## Link against JUCE audio processors target. When JUCE is added as a subdirectory
## the module targets are namespaced as `juce::juce_audio_processors`. When found
## via `find_package` they may be exported as `JUCE::juce_audio_processors`. Try
## the modern `juce::` target first and fall back to `JUCE::` if necessary.
if (TARGET juce::juce_audio_processors)
    target_link_libraries(jamulus_vst3 PRIVATE jamulus_wrapper juce::juce_audio_processors)
elseif (TARGET JUCE::juce_audio_processors)
    target_link_libraries(jamulus_vst3 PRIVATE jamulus_wrapper JUCE::juce_audio_processors)
else()
    message(FATAL_ERROR "Could not find a JUCE audio_processors target. Ensure JUCE is available via add_subdirectory or find_package.")
endif()

# Link Qt for the plugin editor GUI
find_package(Qt6 COMPONENTS Core Widgets Gui QUIET)
if (TARGET Qt6::Widgets)
    target_link_libraries(jamulus_vst3 PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui)
endif()

# Prevent JUCE from erroring when building only the VST3 client in this headless
# wrapper build. Disable VST3 replacement of VST2 parameter IDs (we only ship
# VST3 here) to avoid automation ID mismatch checks in JUCE sources.
target_compile_definitions(jamulus_vst3 PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)

# Ensure the platform-specific plugin artefact targets link the wrapper too.
# JUCE generates additional targets like `<name>_VST3`; add the wrapper
# dependency if those targets exist so static symbols (e.g. ASIO shim)
# are available at final link time.
if (TARGET jamulus_vst3_VST3)
    target_link_libraries(jamulus_vst3_VST3 PRIVATE jamulus_wrapper)
    
    # Embed a Windows manifest that enables DLL redirection so Qt DLLs can be
    # found in the same directory as the plugin module. This is critical for
    # VST3 hosts that don't add the plugin's directory to the DLL search path.
    if (WIN32)
        set(MANIFEST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/JamulusVST3.manifest")
        if (EXISTS "${MANIFEST_FILE}")
            # Use mt.exe post-build to embed the manifest
            # The -outputresource option takes a path and resource ID without space
            add_custom_command(TARGET jamulus_vst3_VST3 POST_BUILD
                COMMAND mt.exe -nologo -manifest "${MANIFEST_FILE}" "-outputresource:$<TARGET_FILE:jamulus_vst3_VST3>;#2"
                COMMENT "Embedding DLL redirection manifest into plugin"
                VERBATIM
            )
        endif()
    endif()
endif()
if (TARGET jamulus_vst3_VST2)
    target_link_libraries(jamulus_vst3_VST2 PRIVATE jamulus_wrapper)
endif()

target_include_directories(jamulus_vst3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(jamulus_vst3 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Generate a minimal VST3 manifest and install it into the built VST3 target directory.
# Some JUCE/CMake configurations don't produce a manifest.ttl automatically; add
# a small post-build step to place a minimal manifest alongside the plugin binary.
if (TARGET jamulus_vst3_VST3)
    # Ensure a manifest.ttl is generated from moduleinfo.json (in case helper output is incomplete).
    set(MODULEINFO "$<TARGET_FILE_DIR:jamulus_vst3_VST3>/../Resources/moduleinfo.json")
    set(MANIFEST_OUT "$<TARGET_FILE_DIR:jamulus_vst3_VST3>/manifest.ttl")
    add_custom_command(TARGET jamulus_vst3_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Generating manifest.ttl from moduleinfo.json"
        COMMAND ${CMAKE_COMMAND} -E env "PYTHONIOENCODING=UTF-8" python "${CMAKE_CURRENT_SOURCE_DIR}/generate_vst3_manifest.py" "${CMAKE_BINARY_DIR}/plugin/jamulus_vst3_artefacts/Release/VST3/JamulusVST3.vst3/Contents/Resources/moduleinfo.json" "${CMAKE_BINARY_DIR}/plugin/jamulus_vst3_artefacts/Release/VST3/JamulusVST3.vst3/Contents/x86_64-win/manifest.ttl"
        COMMENT "Generating VST3 manifest from moduleinfo.json"
    )
endif()
