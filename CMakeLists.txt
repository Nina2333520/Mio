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
)

target_sources(MioNo PRIVATE
    Source/Main.cpp
)

# 生成 JuceHeader.h（源码里 #include <JuceHeader.h> 必须有这一步）
juce_generate_juce_header(MioNo)

target_compile_definitions(MioNo PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:MioNo,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:MioNo,JUCE_VERSION>"
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_MODAL_LOOPS_PERMITTED=0
    # Windows 音频后端
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=0
    # ASIO 由下面的 WIN32 段按需打开（需要 Steinberg ASIO SDK）
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
# Windows：ASIO 支持 + 必要系统库
# ---------------------------------------------------------------------------
if (WIN32)
    # ASIO 头文件目录（若仓库里带了 ASIO SDK 就启用；没有就回退到 WASAPI）
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_audio_devices/native/asio")
        target_compile_definitions(MioNo PRIVATE JUCE_ASIO=1)
        message(STATUS "ASIO SDK 已找到，编译时启用 ASIO")
    else()
        message(STATUS "未找到 ASIO SDK，仅启用 WASAPI")
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
