# MioNo

苹果风的轻量声卡机架 —— 纯 C++ / JUCE 实现。

## 特点

- **体积小**：编译产物约 10~15 MB（对比 Python 版的 47 MB）
- **内存低**：运行占用约 40~70 MB
- **原生 ASIO**：Windows 下直接驱动 ASIO 声卡，低延迟
- **苹果风界面**：无边框窗口、圆角卡片、红黄绿标题栏圆点与透明底完整动漫人物，按原图比例显示、不加黑底、不做圆形裁切，**布局与 Python 版一致**
  （① 设备卡 → ② 预设卡 → ③ 标签页「主效果链 / 空灵层」→ 状态栏）
- **10 种效果器**（都带图标）：🧹谱减降噪 / 🚪噪声门 / 🔺高通 / 🔻低通 / 🎛峰值EQ /
  🗜压缩器 / 🌌混响 / ⏱延迟 / ✨八度叠加 / 🛡限幅
- **4 套内置预设 + 1 个自定义槽**（一键切换）：💬聊天·游戏 / 🎤唱歌 / 🌌空灵 / 📻电台厚声 / ⭐自定义
- **空灵层默认开启** —— 每套内置预设切换后都会启用空灵层；需要干声时可去「✨ 空灵层」标签页关闭
- **自定义预设自动保存**：编辑效果链或空灵层会自动切到「⭐ 自定义」，内置预设与自定义效果链互不覆盖；输入/输出增益、声像和总旁通是全局控制，不会在切预设时突然跳变。升级后旧内置预设自动采用空灵层开启的新默认，自定义预设仍保留原开关状态
- **启动时自动开流**：打开 MioNo 后自动启动上次选中的输入/输出设备；设备暂不可用时会提示原因，可取消勾选关闭
- **Voicemeeter 一键路由**：点输入设备行的「🔀 Banana 路由」，自动切到 WASAPI、保留当前麦克风、优先选择 Banana VAIO 虚拟输入并启动；没有 Banana 时可回退到 VB-CABLE
- **可以缩到状态栏**：关掉窗口不退出，缩到系统托盘继续跑；Windows 登录自启也是直接缩到状态栏

## 界面（跟 Python 版一一对应）

```
┌──────────────────────────────────────────────┐
│ 🔴🟡🟢  [动漫头像] MioNo v6.9 ● ASIO  ● 运行 │  ← 自绘标题栏
├──────────────────────────────────────────────┤
│ ① 设备                                        │
│   输入 [___________]  [🔄 刷新]               │
│   输出 [___________]                          │
│   驱动[___] 采样率[___] 块大小[___]           │
│   输入增益[—●—] 输出增益[—●—] 声像[—●—] 居中  │
│   ☐旁通 ☐开机自启 ☐启动时自动开流 ☐关闭缩到托盘  │
│   [⇩ 缩到托盘]                      [▶ 启动]  │
│   输入 ▓▓▓░░ ▓▓░░░ (CLIP)  峰 -12.3 / 均 -18 │
│   输出 ▓▓░░░ ▓░░░░ (CLIP)  峰 -9.8 / 均 -15  │
├──────────────────────────────────────────────┤
│ ② 预设 / 配置                                 │
│   [💬聊天][🎤唱歌][🌌空灵][📻电台][⭐自定义] │
│                          [💾 保存配置][📂 载入]│
├──────────────────────────────────────────────┤
│ ╭ 🎤 主效果链 ╮╭ ✨ 空灵层 ╮                  │
│ │ ☑ 🧹 谱减降噪 │  强度  [—●——] 0.45         │
│ │ ☑ 🔺 高通     │                            │
│ │ ☑ 🗜 压缩器   │                            │
│ │ [类型▼][＋添加]│                            │
│ │ [↑上移][↓下移][🗑删除]                      │
│ ╰───────────────╯                            │
│ 未开流 · 选好输入/输出后点「▶ 启动」           │
└──────────────────────────────────────────────┘
```

> 点列表最左边那个**小圆点**就是**启用 / 旁通**这一个效果（跟 Python 版一样）。


## 状态栏（托盘）

程序常驻在**右下角状态栏**里，窗口关掉也不影响出声。

| 操作 | 结果 |
|---|---|
| **左键点图标** | 把主窗口叫回来 |
| **右键点图标** | 菜单：显示主窗口 / 启动·停止音频 / 退出 MioNo |
| **点窗口红点**（关闭） | 缩到状态栏（默认）；勾掉「关闭缩到托盘」就是直接退出 |
| **「⇩ 缩到托盘」按钮** | 手动缩到状态栏 |
| **勾「启动时自动开流」** | 每次打开 MioNo（包括 Windows 登录自启）后自动启动已选音频设备；设备失效时提示失败，可关闭此项 |
| **勾「开机自启」** | 写注册表 `HKCU\...\Run`，Windows 登录后**直接缩到状态栏**（不弹窗口） |

> 已经在状态栏里跑着的时候，再双击一次 `MioNo.exe` **不会开出第二个**，
> 而是把原来那个窗口叫回来（音频设备是独占的，开两个也没意义）。

## Voicemeeter Banana / 虚拟声卡一键路由

1. 先安装并打开 Voicemeeter Banana；在 MioNo 输入设备行点「🔀 Banana 路由」。MioNo 会选 WASAPI、沿用当前麦克风，并把输出设为 Voicemeeter 的 VAIO 虚拟输入后启动。
2. 在 Banana 的 VAIO 虚拟输入条上打开 **B1**（发送给聊天/录音软件）；需要自己监听时再打开 **A1**。
3. 在 Discord/OBS 等软件里，把麦克风选为 **Voicemeeter Output（B1）** 对应设备。
4. 如果 Banana 的实体麦克风条也收到原声，关闭该条的 B1 路由，避免干声和处理声叠加。

如果没有检测到 Voicemeeter，可先安装/打开 Banana 再点「🔄 刷新」。检测到 VB-CABLE 时，按钮会改把 MioNo 输出到 **CABLE Input**；此时还要在 Banana 的 Hardware Input 选择 **CABLE Output**。此路由走 WASAPI 的独立输入/输出端点，不使用 Voicemeeter Insert ASIO；声卡仍可由 Windows/WASAPI 使用。设备不匹配时可在设备下拉框手动调整。

## 怎么拿到 exe（不用装任何开发环境）

> 📖 **第一次做的话，看 [`HOWTO-CN.md`](HOWTO-CN.md)** —— 有逐步截图式说明。

1. 去 https://github.com/new 建一个**空仓库**（三个复选框都不要勾）
2. 解压源码，双击 **`push-to-github.bat`**，粘贴仓库地址
3. 进仓库的 **Actions** 标签页，等 `Build MioNo (Windows)` 跑完（首次 4~6 分钟）
4. 点进那次构建，在页面底部 **Artifacts** 里下载 `MioNo-win64.zip`
5. 解压后若里面还有一个 `MioNo-win64.zip`，再解压内层；进入 `MioNo` 文件夹双击 `MioNo.exe`

Windows x64 发布包会把所需的 MSVC/UCRT 运行库 DLL 放在 exe 旁边，Windows 10/11 上无需另装 Visual C++ Redistributable。使用 ASIO 时仍需安装声卡或虚拟声卡厂商提供的 ASIO 驱动。自动恢复状态保存在用户数据目录；“保存配置”会另存为独立 JSON，“载入配置”每次让你选择文件，不会覆盖自动恢复文件。配置也会记住驱动、输入/输出设备、采样率和缓冲块大小；设备不存在时会回退到可用设备。

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
├── assets/avatar.png           # 标题栏动漫头像素材
├── assets/tray-icon.png        # 从 app.ico 抽出的 32×32（托盘图标用）
└── Source/
    ├── Main.cpp                # 入口（--tray 时直接缩到状态栏）
    ├── MainWindow.h            # 主窗口（① 设备卡 / ② 预设卡 / ③ 标签页）+ 托盘
    ├── TrayIcon.h              # 状态栏图标 + 右键菜单
    ├── AppIconData.h           # 内嵌的程序图标字节（自动生成，别手改）
    ├── AvatarData.h            # 内嵌的标题栏头像字节（自动生成，别手改）
    ├── AppleLookAndFeel.h      # 苹果风 LookAndFeel + 配色 + 标签页样式
    ├── AppleUI.h               # 标题栏 / 电平条 / 效果链列表 / 链编辑页
    ├── Rack.h                  # 机架核心 + 预设 + 配置存盘 + 开机自启
    └── FxEngine.h              # 全部 DSP 实现
```

## 音频后端

Windows 构建包含 **ASIO 和 WASAPI**：设备列表中的驱动类型可切换。启动时如果检测到 ASIO，会优先选中 ASIO；也可手动切换到 WASAPI。使用 ASIO 前需先安装声卡/虚拟声卡厂商提供的 ASIO 驱动。GitHub Actions 从 Steinberg 官方站点获取 ASIO SDK 2.3.4，并在构建前校验 SHA-256；SDK 文件不存放在本仓库中。

本地编译如需 ASIO，请先从 [Steinberg ASIO SDK 页面](https://www.steinberg.net/developers/asiosdk-open/) 获取 SDK 并接受相应许可，再运行：

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64 -DMIONO_ASIO_SDK_DIR="C:/path/to/ASIOSDK/common"
cmake --build build --config Release --parallel
```

不提供 SDK 时，CMake 会保留 WASAPI 并关闭 ASIO；GitHub 发布工作流则会在 ASIO 未实际启用时停止，不发布只有 WASAPI 的误导性产物。

## 依赖

- [JUCE](https://github.com/juce-framework/JUCE) 8.0.7
- [Steinberg ASIO SDK](https://www.steinberg.net/developers/asiosdk-open/) 2.3.4（Windows；构建时从官方站点获取）

## 许可

本项目及 JUCE 8.0.7 按 **GNU AGPLv3** 路线提供；ASIO SDK 按其双许可中的 **GPLv3** 路线使用。完整文本见 `LICENSE` 和 `LICENSE-ASIO-GPL-3.0.txt`，第三方来源、版本及校验信息见 `THIRD_PARTY_NOTICES.txt`。本仓库不重新分发 Steinberg SDK 文件。

使用 AGPLv3 构建/分发前请阅读上述许可；若计划闭源或商业分发，请先取得相应商业许可，不要直接使用此构建路线。
