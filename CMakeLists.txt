# ============================================================================
#  MioNo - 轻量声卡机架
#  基于 JUCE 8，结构参考 JUCE 官方 examples/CMake/GuiApp
# ============================================================================

cmake_minimum_required(VERSION 3.22)

project(MioNo VERSION 6.0.0 LANGUAGES C CXX)

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
# 应用目标
# ---------------------------------------------------------------------------
# ⚠️⚠️ 这里【故意不传 ICON_BIG / ICON_SMALL】——别改回去！
#
# 为什么：JUCE 8 的 _juce_generate_icon() 会在「配置阶段」检查 ICON_BIG 指向的文件
# 是否存在，只要它拿到的不是「真实存在的绝对路径」，就当场报：
#     CMake Error at .../JUCEUtils.cmake:605 (message):
#       Could not find icon file:
# 而且报错里路径经常是空白的（日志里只看到冒号），极难排查 —— 我们在 CI 上被卡了 7 次。
#
# 所以策略改成：
#   ① juce_add_gui_app 完全不传图标，JUCE 就不做任何图标检查；
#   ② Windows 下我们自己生成一个 .rc 把 assets/app.ico 嵌进 exe（见文件末尾）。
#      图标找不到也只是警告，绝不 fail。
juce_add_gui_app(MioNo
    PRODUCT_NAME  "MioNo"
    COMPANY_NAME  "MioNo"
    VERSION       "6.0.0"
    BUNDLE_ID     "com.miono.rack")

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

# ---------------------------------------------------------------------------
# 图标：自己嵌进 exe（绕开 JUCE 的图标检查，永远不会让构建失败）
# ---------------------------------------------------------------------------
# 候选顺序：
#   ① 命令行/CI 传进来的 -DMIONO_ICON_FILE=...
#   ② 仓库里的正式图标 assets/app.ico
#   ③ 仓库里的占位图标 assets/app-placeholder.ico
# 三个都没有 → 只警告，跳过图标，继续编译。
set(_miono_icon_candidates
    "${MIONO_ICON_FILE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/app.ico"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/app-placeholder.ico")

set(MIONO_ICON "")
foreach(_candidate IN LISTS _miono_icon_candidates)
    if ((MIONO_ICON STREQUAL "") AND (EXISTS "${_candidate}"))
        get_filename_component(MIONO_ICON "${_candidate}" ABSOLUTE)
    endif()
endforeach()

message(STATUS "===== MioNo 配置诊断 =====")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR = ${CMAKE_CURRENT_BINARY_DIR}")
foreach(_candidate IN LISTS _miono_icon_candidates)
    if (EXISTS "${_candidate}")
        set(_candidate_exists YES)
    else()
        set(_candidate_exists NO)
    endif()
    message(STATUS "图标候选 存在=${_candidate_exists}  [${_candidate}]")
endforeach()
message(STATUS "最终选中的图标 = [${MIONO_ICON}]")
message(STATUS "==========================")

if (WIN32)
    if (MIONO_ICON STREQUAL "")
        message(WARNING "没有找到任何 .ico 文件，跳过图标（不影响编译）")
    else()
        # 用 configure_file 而不是 file(WRITE)：模板里的 @MIONO_ICON_NATIVE@ 是
        # 原样替换（不做转义），所以带反斜杠的 Windows 路径不会出问题。
        file(TO_NATIVE_PATH "${MIONO_ICON}" MIONO_ICON_NATIVE)
        set(_miono_rc_template "${CMAKE_CURRENT_SOURCE_DIR}/assets/app-icon.rc.in")
        if (EXISTS "${_miono_rc_template}")
            configure_file("${_miono_rc_template}"
                           "${CMAKE_CURRENT_BINARY_DIR}/MioNoIcon.rc" @ONLY)
            target_sources(MioNo PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/MioNoIcon.rc")
            message(STATUS "已生成图标资源脚本: ${CMAKE_CURRENT_BINARY_DIR}/MioNoIcon.rc")
            message(STATUS "嵌入的图标: ${MIONO_ICON_NATIVE}")
            # 这行会被 CI 抓出来显示成注解，方便在网页上一眼看到用了哪个图标
            message(STATUS "MIONO_DIAG_ICON=${MIONO_ICON_NATIVE}")
        else()
            message(WARNING "缺少图标资源模板 ${_miono_rc_template}，跳过图标（不影响编译）")
        endif()
    endif()
endif()
