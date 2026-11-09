cmake_minimum_required(VERSION 3.22)

# Mac: build universal binaries (Apple Silicon + Intel). Must be set before project().
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

project(SwellAdLibFX VERSION 1.1.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows: link the C++ runtime statically so the plugin never depends on the
# Visual C++ Redistributable installed on the user's machine
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)
# Monocypher (BSD-2 / CC0) : vérification Ed25519 des fichiers de licence.
# Pas de CMakeLists chez Monocypher : SOURCE_SUBDIR inexistant = télécharger seulement, la cible est déclarée ici.
FetchContent_Declare(monocypher GIT_REPOSITORY https://github.com/LoupVaillant/Monocypher.git GIT_TAG 4.0.3 GIT_SHALLOW TRUE SOURCE_SUBDIR none)
FetchContent_MakeAvailable(monocypher)
add_library(monocypher STATIC "${monocypher_SOURCE_DIR}/src/monocypher.c" "${monocypher_SOURCE_DIR}/src/optional/monocypher-ed25519.c")
target_include_directories(monocypher PUBLIC "${monocypher_SOURCE_DIR}/src" "${monocypher_SOURCE_DIR}/src/optional")
set_target_properties(monocypher PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(SWELL_FORMATS Standalone VST3)
option(SWELL_COPY_AFTER_BUILD "Copy plug-ins into the user audio folders" ${APPLE})
if(APPLE)
    list(APPEND SWELL_FORMATS AU)
endif()

juce_add_plugin(SwellAdLibFX
    COMPANY_NAME "Swell-audio"
    BUNDLE_ID com.swell.adlibfx
    PLUGIN_MANUFACTURER_CODE Swel
    PLUGIN_CODE Adlb
    FORMATS ${SWELL_FORMATS}
    PRODUCT_NAME "Swell Ad-lib FX"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${SWELL_COPY_AFTER_BUILD}
    VST3_CATEGORIES Fx
    AU_MAIN_TYPE kAudioUnitType_Effect)

juce_add_binary_data(SwellAdLibFXData SOURCES
    Resources/swell_logo.png)

target_sources(SwellAdLibFX PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp Source/Licence/SwellLicence.cpp)

target_compile_definitions(SwellAdLibFX PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

# Offline demo renderer: runs a dry vocal through every preset (see tools/RenderDemo.cpp)
juce_add_console_app(SwellAdLibRender PRODUCT_NAME "SwellAdLibRender")
target_sources(SwellAdLibRender PRIVATE tools/RenderDemo.cpp)
target_compile_definitions(SwellAdLibRender PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(SwellAdLibRender
    PRIVATE
        juce::juce_audio_formats
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

target_link_libraries(SwellAdLibFX
    PRIVATE monocypher juce::juce_cryptography
        SwellAdLibFXData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

option(SWELL_BUILD_TESTS "Build audio processing regression checks" OFF)
if(SWELL_BUILD_TESTS)
    enable_testing()
    add_executable(SwellAdLibChecks Tests/AudioChecks.cpp)
    target_link_libraries(SwellAdLibChecks PRIVATE SwellAdLibFX)
    target_include_directories(SwellAdLibChecks PRIVATE Source $<TARGET_PROPERTY:SwellAdLibFX,INCLUDE_DIRECTORIES>)
    target_compile_definitions(SwellAdLibChecks PRIVATE $<TARGET_PROPERTY:SwellAdLibFX,COMPILE_DEFINITIONS>)
    add_test(NAME AdLibAudioChecks COMMAND SwellAdLibChecks)
endif()
