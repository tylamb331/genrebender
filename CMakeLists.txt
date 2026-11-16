cmake_minimum_required(VERSION 3.22)
project(GenreBender VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
endif()

# JUCE: use a local checkout if given (-DJUCE_DIR=...), otherwise download it.
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} juce)
else()
    include(FetchContent)
    FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(juce)
endif()

set(GB_FORMATS VST3)
if(APPLE)
    list(APPEND GB_FORMATS AU)
endif()

juce_add_plugin(GenreBender
    COMPANY_NAME "Sir Lord Black"
    COMPANY_WEBSITE ""
    BUNDLE_ID "com.sirlordblack.genrebender"
    PLUGIN_MANUFACTURER_CODE SlBk
    PLUGIN_CODE GnBd
    PRODUCT_NAME "Genre Bender"
    FORMATS ${GB_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Generator
    AU_MAIN_TYPE kAudioUnitType_MusicDevice)

target_sources(GenreBender PRIVATE
    Source/Generator.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(GenreBender PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0)

if(UNIX AND NOT APPLE)
    # test builds on Linux only: the build box has no Xrandr/Xinerama/Xcursor headers
    target_compile_definitions(GenreBender PUBLIC JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XRENDER=0 JUCE_ALSA=0 JUCE_JACK=0)
endif()

target_link_libraries(GenreBender
    PRIVATE juce::juce_audio_utils juce::juce_gui_extra
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# ---- tests (Linux build box only): links the plugin code directly and also loads the built VST3 ----
option(GB_TESTS "Build the test app" OFF)
if(GB_TESTS)
    juce_add_console_app(GBTests PRODUCT_NAME "GBTests")
    target_sources(GBTests PRIVATE tests/host_test.cpp Source/Generator.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(GBTests PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XRENDER=0 JUCE_ALSA=0 JUCE_JACK=0)
    target_link_libraries(GBTests PRIVATE juce::juce_audio_utils juce::juce_gui_extra juce::juce_recommended_config_flags)
endif()
