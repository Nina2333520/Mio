# MioNo

苹果风的轻量声卡机架 —— 纯 C++ / JUCE 实现。

## 特点

- **体积小**：编译产物约 10~15 MB（对比 Python 版的 47 MB）
- **内存低**：运行占用约 40~70 MB
- **原生 ASIO**：Windows 下直接驱动 ASIO 声卡，低延迟
- **苹果风界面**：无边框窗口、圆角卡片、红黄绿标题栏圆点
- **10 种效果器**：谱减降噪 / 噪声门 / 高通 / 低通 / 峰值EQ / 压缩器 / 混响 / 延迟 / 八度叠加 / 限幅
- **4 套预设**：聊天·游戏 / 唱歌 / 空灵 / 电台厚声
- **可以缩到状态栏**：关掉窗口不退出，缩到系统托盘继续跑；开机自启也是直接缩到状态栏

## 状态栏（托盘）

程序常驻在**右下角状态栏**里，窗口关掉也不影响出声。

| 操作 | 结果 |
|---|---|
| **左键点图标** | 把主窗口叫回来 |
| **右键点图标** | 菜单：显示主窗口 / 启动·停止音频 / 退出 MioNo |
| **点窗口红点**（关闭） | 缩到状态栏（默认）；勾掉「关闭缩到托盘」就是直接退出 |
| **「⇩ 缩到托盘」按钮** | 手动缩到状态栏 |
| **勾「开机自启」** | 写注册表 `HKCU\...\Run`，开机后**直接缩到状态栏**（不弹窗口） |

> 已经在状态栏里跑着的时候，再双击一次 `MioNo.exe` **不会开出第二个**，
> 而是把原来那个窗口叫回来（音频设备是独占的，开两个也没意义）。

## 怎么拿到 exe（不用装任何开发环境）

> 📖 **第一次做的话，看 [`HOWTO-CN.md`](HOWTO-CN.md)** —— 有逐步截图式说明。

1. 去 https://github.com/new 建一个**空仓库**（三个复选框都不要勾）
2. 解压源码，双击 **`push-to-github.bat`**，粘贴仓库地址
3. 进仓库的 **Actions** 标签页，等 `Build MioNo (Windows)` 跑完（首次 4~6 分钟）
4. 点进那次构建，在页面底部 **Artifacts** 里下载 `MioNo-win64.zip`
5. 解压 → 双击 `MioNo.exe`

> 首次构建要下载并编译 JUCE，大约 4~6 分钟；之后有缓存，1~2 分钟。

## 本地编译（可选）

需要：CMake ≥ 3.22、Visual Studio 2022（含 C++ 桌面开发）、Git。

```bash
git clone --recursive https://github.com/<你的用户名>/MioNo.git
cd MioNo
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

产物在 `build/MioNo_artefacts/Release/MioNo.exe`。

## 目录结构

```
MioNoCpp/
├── CMakeLists.txt              # 构建配置
├── .github/workflows/build.yml # GitHub Actions 自动编译
├── assets/app.ico              # 程序图标（exe 图标 + 托盘图标都来自它）
├── assets/tray-icon.png        # 从 app.ico 抽出的 32×32（托盘图标用）
└── Source/
    ├── Main.cpp                # 入口（--tray 时直接缩到状态栏）
    ├── MainWindow.h            # 主窗口（无边框苹果风）+ 托盘 / 关窗口行为
    ├── TrayIcon.h              # 状态栏图标 + 右键菜单
    ├── AppIconData.h           # 内嵌的图标字节（自动生成，别手改）
    ├── AppleLookAndFeel.h      # 苹果风 LookAndFeel + 配色
    ├── AppleUI.h               # 标题栏 / 电平条 / 效果链列表
    ├── Rack.h                  # 机架核心 + 预设 + 配置存盘 + 开机自启
    └── FxEngine.h              # 全部 DSP 实现
```

## 依赖

只有 [JUCE](https://github.com/juce-framework/JUCE)（8.0.7），通过 git submodule 引入。没有其他第三方库。

## 许可

JUCE 采用 AGPLv3 / 商业双许可。个人开源使用走 AGPLv3 即可。
