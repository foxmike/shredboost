cmake_minimum_required(VERSION 3.22)

project(ShredBoost VERSION 1.0.0 LANGUAGES C CXX)

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
        Source/DSP/TightFilter.cpp
        Source/DSP/MidVoicingFilter.cpp
        Source/DSP/InputTrimStage.cpp
)

target_link_libraries(ShredBoost
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# -----------------------------------------------------------------------------
# Unit tests (juce::UnitTest only, no third-party frameworks)
# -----------------------------------------------------------------------------
enable_testing()

add_executable(ShredBoostUnitTests
    tests/TestMain.cpp
    tests/ParameterTests.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters/ParameterLayout.cpp
    Source/Parameters/ParameterSnapshot.cpp
)

target_compile_definitions(ShredBoostUnitTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_include_directories(ShredBoostUnitTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ShredBoostUnitTests PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_basics
    juce::juce_gui_basics
    juce::juce_data_structures
    juce::juce_events
    juce::juce_core
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_test(NAME ShredBoostParameterTests COMMAND ShredBoostUnitTests)

# -----------------------------------------------------------------------------
# DSP filter/stage unit tests (juce::UnitTest console runner, no third-party
# frameworks). Compiled into a dedicated ShredBoostTests executable that runs
# only the FilterTests via the shared tests/TestMain.cpp runner.
# -----------------------------------------------------------------------------
add_executable(ShredBoostTests
    tests/TestMain.cpp
    tests/FilterTests.cpp
    tests/WaveshaperTests.cpp
    tests/OversamplingTests.cpp
    Source/DSP/TightFilter.cpp
    Source/DSP/MidVoicingFilter.cpp
    Source/DSP/InputTrimStage.cpp
    Source/DSP/NonlinearClipper.cpp
    Source/DSP/DcBlocker.cpp
    Source/DSP/OversamplingEngine.cpp
    Source/DSP/AutoGainBypass.cpp
)

target_compile_definitions(ShredBoostTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_include_directories(ShredBoostTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ShredBoostTests PRIVATE
    juce::juce_audio_basics
    juce::juce_dsp
    juce::juce_core
    juce::juce_events
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_test(NAME FilterTests COMMAND ShredBoostTests)
add_test(NAME WaveshaperTests COMMAND ShredBoostTests)
add_test(NAME OversamplingTests COMMAND ShredBoostTests)
