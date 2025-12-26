cmake_minimum_required(VERSION 3.16)
project(jamulus_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Core Network Widgets Xml Concurrent Gui LinguistTools QUIET)
# Qt Multimedia is optional in some Qt builds; try to locate it but do not fail if missing.
find_package(Qt6 COMPONENTS Multimedia QUIET)

set(JAMULUS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../")

# Enable Qt auto tools so .ui/.moc/.qrc are handled automatically
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Collect all source files from upstream Jamulus src directory
file(GLOB_RECURSE ALL_JAMULUS_SOURCES
    ${JAMULUS_ROOT}/src/*.cpp
    ${JAMULUS_ROOT}/src/*/*.cpp
)

# Filter sources to platform-relevant files to avoid pulling in mac/android-only code
set(JAMULUS_SOURCES_ABS)
foreach(f IN LISTS ALL_JAMULUS_SOURCES)
    string(REPLACE "${JAMULUS_ROOT}/" "" rel ${f})
    string(FIND "${rel}" "sound/coreaudio-mac" pos_coreaudio)
    string(FIND "${rel}" "sound/oboe" pos_oboe)
    string(FIND "${rel}" "sound/oboe" pos_oboe2)
    string(FIND "${rel}" "android/" pos_android)
    string(FIND "${rel}" "ios/" pos_ios)
    string(FIND "${rel}" "recorder/" pos_recorder)
    string(FIND "${rel}" "plugins/" pos_plugins)
    string(FIND "${rel}" "sound/jack" pos_jack)
    string(FIND "${rel}" "sound/asio" pos_asio)
    # Exclude platform-specific/backends we don't want to build on Windows
    if (pos_coreaudio EQUAL -1 AND pos_oboe EQUAL -1 AND pos_android EQUAL -1 AND pos_ios EQUAL -1 AND pos_jack EQUAL -1)
        # Skip upstream ASIO sound implementation; we'll provide a lightweight
        # `CSound` stub in the wrapper overrides to avoid pulling in ASIO
        # driver dependencies into the plugin binary.
        if (pos_asio EQUAL -1)
            list(APPEND JAMULUS_SOURCES_ABS ${f})
        endif()
    endif()
endforeach()

add_library(jamulus_core STATIC ${JAMULUS_SOURCES_ABS})

# If any override sources are provided (e.g. lightweight CSound stub), add them
# so they can replace upstream platform-specific implementations without
# modifying upstream sources.
file(GLOB JAMULUS_OVERRIDE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/overrides/*.cpp)
if(JAMULUS_OVERRIDE_SOURCES)
    target_sources(jamulus_core PRIVATE ${JAMULUS_OVERRIDE_SOURCES})
    target_include_directories(jamulus_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/overrides)
    message(STATUS "Adding jamulus override sources: ${JAMULUS_OVERRIDE_SOURCES}")
endif()

# Build bundled Opus library and link into jamulus_core so opus symbols are available
# Enable Opus custom modes needed by Jamulus (custom codec APIs)
set(OPUS_CUSTOM_MODES ON CACHE BOOL "Enable Opus custom modes required by Jamulus" FORCE)
add_subdirectory(${JAMULUS_ROOT}/libs/opus ${CMAKE_BINARY_DIR}/libs_opus EXCLUDE_FROM_ALL)
target_link_libraries(jamulus_core PRIVATE opus)

option(ASIO_SDK_DIR "Path to ASIO SDK include directory" "")

target_include_directories(jamulus_core PUBLIC
    "${JAMULUS_ROOT}/src"
    "${JAMULUS_ROOT}/libs/opus/include"
    "${JAMULUS_ROOT}/libs/opus"
    "${JAMULUS_ROOT}/libs"
)

# always add stubs (e.g. unistd.h) so moc and compiler can find them
if(ASIO_SDK_DIR)
    # Prefer the real ASIO SDK include directory when provided
    # Add ASIO SDK first, then local stubs so unistd.h and other helpers are available
    target_include_directories(jamulus_core PUBLIC ${ASIO_SDK_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/include_stub")
    message(STATUS "Using ASIO SDK includes from: ${ASIO_SDK_DIR} (include_stub also added)")
else()
    # No ASIO SDK provided: fall back to local stub headers (no real ASIO)
    target_include_directories(jamulus_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include_stub")
    message(STATUS "ASIO_SDK_DIR not set — using local stub headers in include_stub (no real ASIO)")
endif()

set(JAMULUS_QT_LIBS)
if (TARGET Qt6::Core)
    list(APPEND JAMULUS_QT_LIBS Qt6::Core Qt6::Network Qt6::Widgets Qt6::Xml Qt6::Concurrent Qt6::Gui)
    if (TARGET Qt6::Multimedia)
        list(APPEND JAMULUS_QT_LIBS Qt6::Multimedia)
    endif()
elseif (TARGET Qt5::Core)
    list(APPEND JAMULUS_QT_LIBS Qt5::Core Qt5::Network Qt5::Widgets Qt5::Xml Qt5::Concurrent)
endif()

target_link_libraries(jamulus_core PRIVATE ${JAMULUS_QT_LIBS})

# Provide commonly expected defines from qmake build
target_compile_definitions(jamulus_core PRIVATE APP_VERSION=\"0.0.0\" NOMINMAX)

# Note: upstream Jamulus expects the ASIO SDK headers on Windows. If not available,
# provide a minimal stub header in include_stub to allow compilation (functionality
# will be limited). See include_stub/asiosys.h

message(STATUS "jamulus_core: ${JAMULUS_SOURCES_ABS}")
