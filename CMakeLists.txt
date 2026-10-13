# ============================================================================
#  MioNo - 轻量声卡机架
#  基于 JUCE 8，结构参考 JUCE 官方 examples/CMake/GuiApp
# ============================================================================

cmake_minimum_required(VERSION 3.22)

project(MioNo VERSION 6.7.0 LANGUAGES C CXX)

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
    VERSION       "6.7.0"
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

# ---------------------------------------------------------------------------
# MSVC：源码是 UTF-8，必须显式告诉编译器
# ---------------------------------------------------------------------------
# ⚠️ 不加 /utf-8 的话，MSVC 会按「系统 ANSI 代码页」读源码，
#    界面上的中文会全变成 è”±ä¸Šè§’æœ¬ 这种乱码（我们在 CI 上踩过）。
#    另外注意：JUCE 的 String(const char*) 是按 ASCII 处理的，
#    所以源码里的中文一律用 L"..." 宽字符串（UTF-16），不要用 u8"..."。
if (MSVC)
    target_compile_options(MioNo PRIVATE /utf-8)
endif()

# 链接 JUCE 模块
# 体积优化：只留真正用到的模块。去掉 juce_audio_processors（不做 VST 宿主）、
# juce_audio_utils（界面控件集，没用到）。
# 注意：
#   · juce_audio_formats 不能去：juce_dsp 依赖它。
#   · juce_gui_extra 必须留着：SystemTrayIconComponent（状态栏图标）在这个模块里。
target_link_libraries(MioNo
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
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
# Windows：ASIO + WASAPI
# ---------------------------------------------------------------------------
# Steinberg SDK headers are fetched by CI into the runner's temporary directory;
# never commit third-party ASIO SDK files into this public repository.
# Local builds may pass -DMIONO_ASIO_SDK_DIR=<.../ASIOSDK/common>.
option(MIONO_ENABLE_ASIO "Enable ASIO when the Steinberg SDK headers are available" ON)
set(MIONO_ASIO_SDK_DIR "" CACHE PATH "Path to Steinberg ASIO SDK common headers")

if (WIN32)
    if (MIONO_ASIO_SDK_DIR STREQUAL "")
        set(_miono_local_asio "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/ASIOSDK/common")
        if (EXISTS "${_miono_local_asio}/iasiodrv.h")
            set(MIONO_ASIO_SDK_DIR "${_miono_local_asio}")
        endif()
    endif()

    if (MIONO_ENABLE_ASIO AND EXISTS "${MIONO_ASIO_SDK_DIR}/iasiodrv.h")
        target_compile_definitions(MioNo PRIVATE JUCE_ASIO=1)
        target_include_directories(MioNo PRIVATE "${MIONO_ASIO_SDK_DIR}")
        set(MIONO_ASIO_ENABLED ON)
        message(STATUS "MIONO_DIAG_ASIO=ON (${MIONO_ASIO_SDK_DIR})")
    else()
        target_compile_definitions(MioNo PRIVATE JUCE_ASIO=0)
        set(MIONO_ASIO_ENABLED OFF)
        message(STATUS "MIONO_DIAG_ASIO=OFF (WASAPI fallback remains enabled)")
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
        # 做法跟 JUCE 自己一样：把图标拷进 build 目录，.rc 里只写【文件名】。
        # 千万别在这里写完整路径 —— .rc 里的字符串会走 C 风格的转义，
        # "C:\a\Mio\..." 里的 \a \M 会被吃掉，路径就废了（rc.exe 报找不到文件）。
        get_filename_component(_miono_icon_name "${MIONO_ICON}" NAME)
        file(COPY "${MIONO_ICON}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
        set(_miono_rc "${CMAKE_CURRENT_BINARY_DIR}/MioNoIcon.rc")
        file(WRITE "${_miono_rc}"
            "// 由 CMake 自动生成：把应用图标嵌进 MioNo.exe\n"
            "IDI_ICON1 ICON DISCARDABLE \"${_miono_icon_name}\"\n")
        target_sources(MioNo PRIVATE "${_miono_rc}")
        message(STATUS "已生成图标资源脚本: ${_miono_rc}")
        message(STATUS "图标文件已复制到: ${CMAKE_CURRENT_BINARY_DIR}/${_miono_icon_name}")
        # 这行会被 CI 抓出来显示成注解，方便在网页上一眼看到用了哪个图标
        message(STATUS "MIONO_DIAG_ICON=${CMAKE_CURRENT_BINARY_DIR}/${_miono_icon_name}")
    endif()
endif()
