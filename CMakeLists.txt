cmake_minimum_required(VERSION 3.22)

project(ShredBoost VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(
    SHREDBOOST_CONFIGURE_ONLY
    "Configure the top-level project without requiring the JUCE submodule"
    OFF
)

if(SHREDBOOST_CONFIGURE_ONLY)
    message(STATUS "ShredBoost configure-only mode enabled")
    return()
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    message(
        FATAL_ERROR
        "The pinned JUCE submodule is missing. Populate JUCE before configuring a build."
    )
endif()

add_subdirectory(JUCE)

juce_add_plugin(ShredBoost
    PRODUCT_NAME ShredBoost
    COMPANY_NAME "nSpace Systems"
    VERSION 1.0.0
    PLUGIN_MANUFACTURER_CODE NSPC
    PLUGIN_CODE ShBs
    BUNDLE_ID com.nspacesystems.shredboost
    LV2URI urn:nspacesystems:plugin:shredboost
    FORMATS VST3 LV2 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
)

target_sources(ShredBoost
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_link_libraries(ShredBoost
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
