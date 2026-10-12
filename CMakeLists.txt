# ============================================================================
#  MioNo - 轻量声卡机架
#  基于 JUCE 8，结构参考 JUCE 官方 examples/CMake/GuiApp
# ============================================================================

cmake_minimum_required(VERSION 3.22)

project(MioNo VERSION 6.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE：优先用同级 JUCE 目录，找不到就报清晰的错
# ---------------------------------------------------------------------------
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE/CMakeLists.txt")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" juce)
else()
    message(FATAL_ERROR
        "找不到 JUCE。请在项目根目录执行：\n"
        "  git clone --depth 1 --branch 8.0.7 https://github.com/juce-framework/JUCE.git JUCE")
endif()

# ---------------------------------------------------------------------------
# 图标选择
# ---------------------------------------------------------------------------
# ⚠️ 踩过的坑，别改回去：
# JUCE 的 juce_add_gui_app 里 ICON_BIG 必须传一个【真实存在】的文件路径。
#   - 完全不传 → JUCE 内部属性变成 "JUCE_ICON_BIG-NOTFOUND"，
#     而它在 CMake 里是【真值】，会被当图标路径去检查，直接 FATAL_ERROR。
#     报错长这样：Could not find icon file:      （冒号后面看着是空白）
#   - 传空字符串 → 同样触发上面的问题。
# 因此这里永远传一个存在的文件：首选正式图标，备选占位图标。
set(MIONO_ICON_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/assets/app.ico")
set(MIONO_ICON_FALLBACK "${CMAKE_CURRENT_SOURCE_DIR}/assets/app-placeholder.ico")

if (EXISTS "${MIONO_ICON_MAIN}")
    set(MIONO_ICON "${MIONO_ICON_MAIN}")
    message(STATUS "使用正式图标: ${MIONO_ICON}")
elseif (EXISTS "${MIONO_ICON_FALLBACK}")
    set(MIONO_ICON "${MIONO_ICON_FALLBACK}")
    message(WARNING "正式图标缺失，使用占位图标: ${MIONO_ICON}")
else()
    message(FATAL_ERROR
        "找不到任何图标文件。请确认 assets/ 目录完整：\n"
        "  ${MIONO_ICON_MAIN}\n"
        "  ${MIONO_ICON_FALLBACK}")
endif()

# 统一转成绝对路径（JUCE 内部会做一次，这里先做一次更保险）
get_filename_component(MIONO_ICON_ABS "${MIONO_ICON}" ABSOLUTE)
set(_miono_icon_exists FALSE)
if (EXISTS "${MIONO_ICON_ABS}")
    set(_miono_icon_exists TRUE)
endif()

# ---------------------------------------------------------------------------
# 应用目标
# ---------------------------------------------------------------------------
# 打印诊断信息，方便在 CI 日志里核对
message(STATUS "===== MioNo 配置诊断 =====")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR = ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "选中的图标              = ${MIONO_ICON}")
message(STATUS "图标绝对路径            = ${MIONO_ICON_ABS}")
message(STATUS "图标是否存在            = ${_miono_icon_exists}")
message(STATUS "==========================")

juce_add_gui_app(MioNo
    PRODUCT_NAME  "MioNo"
    COMPANY_NAME  "MioNo"
    VERSION       "6.0.0"
    BUNDLE_ID     "com.miono.rack"
    ICON_BIG      "${MIONO_ICON_ABS}")

# 源码
target_sources(MioNo PRIVATE
    Source/Main.cpp)

# 生成 JuceHeader.h（源码里 #include <JuceHeader.h> 需要它）
juce_generate_juce_header(MioNo)

# 编译定义
target_compile_definitions(MioNo PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:MioNo,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:MioNo,JUCE_VERSION>"
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=0)

target_include_directories(MioNo PRIVATE Source)

# 链接 JUCE 模块
target_link_libraries(MioNo
    PRIVATE
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
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Windows：ASIO 按需启用 + 系统库
# ---------------------------------------------------------------------------
if (WIN32)
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_audio_devices/native/asio")
        target_compile_definitions(MioNo PRIVATE JUCE_ASIO=1)
        message(STATUS "ASIO SDK found - ASIO enabled")
    else()
        message(STATUS "ASIO SDK not found - using WASAPI only")
    endif()

    target_link_libraries(MioNo PRIVATE
        winmm avrt ole32 oleaut32 uuid shell32 shlwapi version user32 gdi32)
endif()
