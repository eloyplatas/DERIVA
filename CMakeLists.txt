cmake_minimum_required(VERSION 3.22)

# macOS: universal binary (set CMAKE_OSX_ARCHITECTURES="arm64;x86_64" when configuring)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
# Windows: static MSVC runtime, so no Visual C++ redistributable is needed
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(Deriva VERSION 1.1.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: a local checkout next to this folder, or downloaded automatically
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" CACHE PATH "Path to JUCE")
if (EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.10
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Deriva
    COMPANY_NAME "Eloy Platas"
    COMPANY_WEBSITE ""
    PLUGIN_MANUFACTURER_CODE Elpl
    PLUGIN_CODE Drva
    FORMATS VST3 Standalone
    PRODUCT_NAME "Deriva"
    DESCRIPTION "Prime-number memory drifter"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Delay Reverb
    BUNDLE_ID com.eloyplatas.deriva)

juce_add_binary_data(DerivaData SOURCES
    Resources/fonts/Montserrat-Regular.ttf
    Resources/fonts/Montserrat-Medium.ttf
    Resources/fonts/Montserrat-SemiBold.ttf
    Resources/fonts/Montserrat-Bold.ttf)

target_sources(Deriva PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

# Portable Linux binary: works on glibc >= 2.35 (Ubuntu 22.04, Debian 12, Fedora 35 and newer) and does not depend on the host's libstdc++
option(DERIVA_PORTABLE_LINUX "Build a Linux binary compatible with older distributions" ON)
if (DERIVA_PORTABLE_LINUX AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(Deriva PRIVATE compat/glibc_compat.c)
    set_source_files_properties(compat/glibc_compat.c PROPERTIES COMPILE_OPTIONS "-std=c17;-U_GNU_SOURCE;-fno-lto")
    foreach(t Deriva Deriva_VST3 Deriva_Standalone)
        if (TARGET ${t})
            target_compile_options(${t} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-include${CMAKE_CURRENT_SOURCE_DIR}/compat/glibc_compat.h>)
            target_link_options(${t} PRIVATE -static-libstdc++ -Wl,--exclude-libs,ALL -Wl,--wrap=fmod -Wl,--wrap=fmodf -Wl,--wrap=arc4random)
        endif()
    endforeach()
endif()

target_include_directories(Deriva PRIVATE Source)

# Beads DSP (Mutable Instruments, MIT) + r8brain resampler (MIT)
set(DERIVA_BEADS_SOURCES
    Source/dsp/BeadsUnit.cpp
    ThirdParty/mi/beads/dsp/processor.cc
    ThirdParty/mi/beads/dsp/mu_law.cc
    ThirdParty/mi/beads/dsp/delay_clock.cc
    ThirdParty/mi/beads/dsp/delay_controller.cc
    ThirdParty/mi/beads/dsp/trigger_generator.cc
    ThirdParty/mi/beads/resources.cc
    ThirdParty/mi/stmlib/utils/random.cc
    ThirdParty/mi/stmlib/dsp/units.cc)
target_sources(Deriva PRIVATE ${DERIVA_BEADS_SOURCES})
set_source_files_properties(${DERIVA_BEADS_SOURCES} PROPERTIES
    COMPILE_DEFINITIONS "TEST;_USE_MATH_DEFINES"
    COMPILE_OPTIONS "$<IF:$<CXX_COMPILER_ID:MSVC>,/w,-w>"
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/mi;${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/r8brain")

target_compile_definitions(Deriva PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Deriva
    PRIVATE
        DerivaData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Development tool: renders the editor with a fed engine to a PNG
option(DERIVA_BUILD_SHOT "Build the GUI snapshot tool" OFF)
if (DERIVA_BUILD_SHOT)
    juce_add_console_app(DerivaShot PRODUCT_NAME "DerivaShot")
    target_sources(DerivaShot PRIVATE tools/shot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp ${DERIVA_BEADS_SOURCES})
    target_include_directories(DerivaShot PRIVATE Source)
    target_compile_definitions(DerivaShot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Deriva")
    target_link_libraries(DerivaShot PRIVATE DerivaData juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
