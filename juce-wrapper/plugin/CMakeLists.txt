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

# ==============================================================================
# 1. THE CORE ENGINE (Renamed from jamulus_vst3 to jamulus_engine)
# ==============================================================================
# Use juce_add_plugin but we will force it to be a plain DLL
# Target name is jamulus_engine to avoid conflict with libjamulus's "jamulus_core"
juce_add_plugin(jamulus_engine
    PRODUCT_NAME "JamulusCore"
    VERSION 0.1
    IS_SYNTH FALSE
    NEEDS_MIDI FALSE
    FORMATS VST3
)

target_sources(jamulus_engine PRIVATE
    PluginProcessor.cpp
    PluginProcessor.h
    PluginEditor.cpp
    PluginEditor.h
)

# Add the JUCE adapter which implements `createPluginFilter()` expected
# by JUCE's plugin client code.
target_sources(jamulus_engine PRIVATE
    JucePluginAdapter.cpp
    JucePluginAdapter.h
)

# Force Dynamic Runtime (MD) to match Qt
set_target_properties(jamulus_engine PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

# Include the local ASIO shim in the plugin shared-code
target_sources(jamulus_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../asioshim.cpp)

## Link against JUCE audio processors target.
if (TARGET juce::juce_audio_processors)
    target_link_libraries(jamulus_engine PRIVATE jamulus_wrapper juce::juce_audio_processors)
elseif (TARGET JUCE::juce_audio_processors)
    target_link_libraries(jamulus_engine PRIVATE jamulus_wrapper JUCE::juce_audio_processors)
else()
    message(FATAL_ERROR "Could not find a JUCE audio_processors target.")
endif()

# Link Qt for the plugin editor GUI
find_package(Qt6 COMPONENTS Core Network QUIET)
if (TARGET Qt6::Core)
    target_link_libraries(jamulus_engine PRIVATE Qt6::Core Qt6::Network)
endif()

# Prevent JUCE from erroring when building only the VST3 client
target_compile_definitions(jamulus_engine PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)

# Ensure the platform-specific plugin artefact targets link the wrapper too.
if (TARGET jamulus_engine_VST3)
    target_link_libraries(jamulus_engine_VST3 PRIVATE jamulus_wrapper)

    # Embed Manifest into Core DLL (for Qt Redirection)
    if (WIN32)
        set(MANIFEST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/JamulusVST3.manifest")
        if (EXISTS "${MANIFEST_FILE}")
            add_custom_command(TARGET jamulus_engine_VST3 POST_BUILD
                COMMAND mt.exe -nologo -manifest "${MANIFEST_FILE}" "-outputresource:$<TARGET_FILE:jamulus_engine_VST3>;#2"
                COMMENT "Embedding DLL redirection manifest into Core DLL"
                VERBATIM
            )
        endif()
    endif()
endif()

if (TARGET jamulus_engine_VST2)
    target_link_libraries(jamulus_engine_VST2 PRIVATE jamulus_wrapper)
endif()

target_include_directories(jamulus_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Set output name to JamulusCore so it produces JamulusCore.dll (inside the bundle)
# We want this to be a plain DLL, typically JUCE VST3 targets generate a bundle structure.
# We will hijack the output so we can copy the .vst3/Contents/x86_64-win/JamulusCore.vst3 -> .dll


# ==============================================================================
# 2. THE BOOTSTRAP SHIM (The new Entry Point)
# ==============================================================================
add_library(jamulus_control MODULE vst3_bootstrap.cpp)

# Static Runtime (/MT), Output "JamulusVST3"
set_target_properties(jamulus_control PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    SUFFIX ".vst3"
    OUTPUT_NAME "JamulusVST3"
)

# ==============================================================================
# 3. POST-BUILD DEPLOYMENT
# ==============================================================================
if (TARGET jamulus_control)
    # Define the Staging Bundle Directory
    set(BUNDLE_DIR "${CMAKE_BINARY_DIR}/plugin/jamulus_shim_bundle/JamulusVST3.vst3")
    set(CONTENTS_DIR "${BUNDLE_DIR}/Contents/x86_64-win")

    # 1. Create the Bundle Structure
    add_custom_command(TARGET jamulus_control POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CONTENTS_DIR}"
        COMMENT "Creating VST3 bundle structure"
    )

    # 2. Copy the Shim Binary (JamulusVST3.vst3) into the bundle
    add_custom_command(TARGET jamulus_control POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:jamulus_control>" "${CONTENTS_DIR}/JamulusVST3.vst3"
        COMMENT "Copying Shim binary to bundle"
    )

    # 3. Copy the Core Engine (JamulusCore.dll) into the bundle
    # Note: We take the output of jamulus_engine_VST3 (which is a .vst3 file) and rename it to .dll
    add_custom_command(TARGET jamulus_control POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:jamulus_engine_VST3>" "${CONTENTS_DIR}/JamulusCore.dll"
        COMMENT "Deploying JamulusCore engine to VST3 bundle"
    )

    # 4. Copy Bundle to Dist
    set(DIST_DIR "${CMAKE_BINARY_DIR}/../dist/JamulusVST3.vst3")
    add_custom_command(TARGET jamulus_control POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${BUNDLE_DIR}" "${DIST_DIR}"
        COMMENT "Copying assembled VST3 bundle to dist: ${DIST_DIR}"
    )

    # 5. Copy Basic Qt DLLs
    set(QT_BIN_DIR "C:/Qt/6.7.3/msvc2022_64/bin")
    add_custom_command(TARGET jamulus_control POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_BIN_DIR}/Qt6Core.dll" "${DIST_DIR}/Contents/x86_64-win/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_BIN_DIR}/Qt6Network.dll" "${DIST_DIR}/Contents/x86_64-win/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_BIN_DIR}/Qt6Xml.dll" "${DIST_DIR}/Contents/x86_64-win/"
        COMMENT "Copying basic Qt DLLs to dist bundle"
    )
endif()
