cmake_minimum_required(VERSION 3.22)

project(MioNo VERSION 6.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE
# ---------------------------------------------------------------------------
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH "JUCE source dir")

if (NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "JUCE not found at ${JUCE_DIR}.\n"
        "Run:  git submodule update --init --recursive\n"
        "or:   git clone --depth 1 --branch 8.0.7 https://github.com/juce-framework/JUCE.git JUCE")
endif()

add_subdirectory("${JUCE_DIR}" EXCLUDE_FROM_ALL)

# ---------------------------------------------------------------------------
# 目标
# ---------------------------------------------------------------------------
juce_add_gui_app(MioNo
    PRODUCT_NAME              "MioNo"
    COMPANY_NAME              "MioNo"
    VERSION                   "6.0.0"
    BUNDLE_ID                 "com.miono.rack"
    ICON_BIG                  "${CMAKE_CURRENT_SOURCE_DIR}/assets/app.ico"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "MioNo 需要麦克风权限来处理你的声音。"
)

target_sources(MioNo PRIVATE
    Source/Main.cpp
)

target_compile_definitions(MioNo PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:MioNo,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:MioNo,JUCE_VERSION>"
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_MODAL_LOOPS_PERMITTED=0
    # 启用 ASIO（Windows）
    JUCE_ASIO=1
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=0
)

target_include_directories(MioNo PRIVATE Source)

target_link_libraries(MioNo PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ---------------------------------------------------------------------------
# Windows：ASIO SDK（若存在）+ 必要库
# ---------------------------------------------------------------------------
if (WIN32)
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_audio_devices/native/asio")
        juce_add_module_sources()
    endif()

    target_link_libraries(MioNo PRIVATE
        winmm avrt ole32 oleaut32 uuid shell32 shlwapi version user32 gdi32)
endif()

# ---------------------------------------------------------------------------
# macOS
# ---------------------------------------------------------------------------
if (APPLE)
    set_target_properties(MioNo PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "MioNo"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.miono.rack")
endif()
