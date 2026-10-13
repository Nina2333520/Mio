// ============================================================================
//  MioNo - 系统托盘（状态栏）图标
//  v6.2：关掉窗口 / 开机自启时，程序缩到状态栏继续跑
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include "AppleLookAndFeel.h"
#include "AppIconData.h"

namespace miono
{

#if JUCE_WINDOWS || JUCE_MAC || JUCE_LINUX || JUCE_BSD

// ---------------------------------------------------------------------------
//  状态栏小图标
//
//  用法（跟普通组件不一样，别 addAndMakeVisible）：
//      TrayIcon tray;                       // 建出来图标就出现在状态栏了
//      tray.onShowWindow = ...;             // 左键单击 / 双击 / 菜单「显示主窗口」
//      tray.onToggleRun  = ...;             // 菜单「启动 / 停止音频」
//      tray.onQuit       = ...;             // 菜单「退出 MioNo」
//      tray.setTip (L"MioNo · 运行中");
//
//  右键 = 弹出菜单；左键 = 把主窗口叫回来。
// ---------------------------------------------------------------------------
class TrayIcon : public juce::SystemTrayIconComponent
{
public:
    std::function<void()> onShowWindow;
    std::function<void()> onToggleRun;
    std::function<void()> onQuit;

    TrayIcon()
    {
        setIconImage (makeIconImage(), makeIconImage());
        setTip (L"MioNo · 未开流");
    }

    /** 悬停提示（同时也是「在不在跑」的可见标志）。内容没变就不打扰系统。 */
    void setTip (const juce::String& t)
    {
        if (t == tip) return;
        tip = t;
        setIconTooltip (tip);
    }

    /** 菜单里那一行「启动 / 停止」的文字跟着状态走。 */
    void setRunning (bool r) { running = r; }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())            // 右键 → 菜单
            showMenu();
        else if (onShowWindow != nullptr)    // 左键 → 叫出窗口
            onShowWindow();
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        if (onShowWindow != nullptr)
            onShowWindow();
    }

    // -----------------------------------------------------------------------
    //  图标图案：优先用内嵌的 App 图标，读不出来就画一个（绝不会没图标）
    // -----------------------------------------------------------------------
    static juce::Image makeIconImage()
    {
        // ① 内嵌的 assets/app.ico（32×32 PNG，编进 exe，不依赖外部文件）
        auto img = juce::ImageFileFormat::loadFrom ((const void*) kAppIconPng,
                                                    (size_t) kAppIconPngSize);
        if (img.isValid())
            return img;

        // ② 兜底：蓝色圆角方块 + 白色 M（跟标题栏那个头像一个风格）
        juce::Image fallback (juce::Image::ARGB, 32, 32, true);
        {
            juce::Graphics g (fallback);
            juce::ColourGradient grad (AppleColors::Acc.brighter (0.28f), 0.0f, 0.0f,
                                       AppleColors::Acc.darker  (0.14f), 0.0f, 32.0f, false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (1.0f, 1.0f, 30.0f, 30.0f, 8.5f);

            g.setColour (juce::Colours::white);
            g.setFont (AppleLookAndFeel::getBoldFont (20.0f));
            g.drawText ("M", fallback.getBounds(), juce::Justification::centred, false);
        }
        return fallback;
    }

private:
    void showMenu()
    {
        // 托盘图标的坐标是无效的，菜单位置要问系统当前鼠标在哪
        const auto p = juce::Desktop::getInstance().getMousePosition();

        juce::PopupMenu m;
        m.addSectionHeader (L"MioNo v6.2");
        m.addItem (1, L"显示主窗口");
        m.addItem (2, running ? L"■ 停止音频" : L"▶ 启动音频");
        m.addSeparator();
        m.addItem (3, L"退出 MioNo");

        m.showMenuAsync (juce::PopupMenu::Options()
                             .withTargetScreenArea (juce::Rectangle<int> (p.x, p.y, 1, 1))
                             .withMinimumWidth (150),
            [this] (int result)
            {
                // 注意：这几个回调里可能会把窗口（连带本对象）销毁掉，
                // 所以「退出」走一次 callAsync，别在本对象的调用栈里直接销毁自己。
                if (result == 1) { if (onShowWindow != nullptr) onShowWindow(); }
                else if (result == 2) { if (onToggleRun != nullptr) onToggleRun(); }
                else if (result == 3)
                {
                    auto cb = onQuit;
                    juce::MessageManager::callAsync ([cb] { if (cb != nullptr) cb(); });
                }
            });
    }

    bool running = false;
    juce::String tip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrayIcon)
};

#endif // 支持的平台

} // namespace miono
