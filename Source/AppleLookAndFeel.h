// ============================================================================
//  MioNo - 苹果风 LookAndFeel
//  把 Python 版 QSS 的视觉语言，全部用 JUCE 的绘制钩子复刻
// ============================================================================
#pragma once

#include <JuceHeader.h>

namespace miono
{

// 苹果配色 —— 与 Python 版 MioNo.py 常量一一对应
namespace AppleColors
{
    static const juce::Colour Bg      { 0xFFF5F5F7 };  // BG
    static const juce::Colour Card    { 0xFFFFFFFF };  // CARD
    static const juce::Colour Card2   { 0xFFFAFAFC };  // CARD2
    static const juce::Colour Line    { 0xFFE3E3E8 };  // LINE
    static const juce::Colour Line2   { 0xFFD5D5DC };  // LINE2
    static const juce::Colour Txt     { 0xFF1D1D1F };  // TXT
    static const juce::Colour Dim     { 0xFF86868B };  // DIM
    static const juce::Colour Acc     { 0xFF007AFF };  // ACC
    static const juce::Colour Acc2    { 0xFF5E5CE6 };  // ACC2
    static const juce::Colour Green   { 0xFF34C759 };  // GREEN
    static const juce::Colour Red     { 0xFFFF3B30 };  // RED
    static const juce::Colour Orange  { 0xFFFF9500 };  // ORANGE
}

class AppleLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AppleLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, AppleColors::Bg);
        setColour (juce::Label::textColourId,                 AppleColors::Txt);
        setColour (juce::TextButton::buttonColourId,          AppleColors::Card);
        setColour (juce::TextButton::textColourOffId,         AppleColors::Txt);
        setColour (juce::TextButton::textColourOnId,          juce::Colours::white);
        setColour (juce::ComboBox::backgroundColourId,        AppleColors::Card);
        setColour (juce::ComboBox::textColourId,              AppleColors::Txt);
        setColour (juce::ComboBox::outlineColourId,           AppleColors::Line);
        setColour (juce::ComboBox::arrowColourId,             AppleColors::Dim);
        setColour (juce::PopupMenu::backgroundColourId,       AppleColors::Card);
        setColour (juce::PopupMenu::textColourId,             AppleColors::Txt);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, AppleColors::Acc);
        setColour (juce::PopupMenu::highlightedTextColourId,  juce::Colours::white);
        setColour (juce::Slider::textBoxTextColourId,         AppleColors::Txt);
        setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
        setColour (juce::ToggleButton::textColourId,          AppleColors::Txt);
        setColour (juce::ToggleButton::tickColourId,          juce::Colours::white);
        setColour (juce::ToggleButton::tickDisabledColourId,  AppleColors::Line2);
        setColour (juce::ScrollBar::thumbColourId,            juce::Colour (0xFFC9C9CE));
        setColour (juce::ListBox::backgroundColourId,         AppleColors::Card);
        setColour (juce::ListBox::textColourId,               AppleColors::Txt);
        setColour (juce::TooltipWindow::backgroundColourId,   AppleColors::Card);
        setColour (juce::TooltipWindow::textColourId,         AppleColors::Txt);
        setColour (juce::TooltipWindow::outlineColourId,      AppleColors::Line);

        // 标签页（QSS: QTabBar::tab{background:#EDEDF0;border-radius:10px 10px 0 0}）
        setColour (juce::TabbedComponent::backgroundColourId,   AppleColors::Bg);
        setColour (juce::TabbedComponent::outlineColourId,      AppleColors::Line);
        setColour (juce::TabbedButtonBar::tabTextColourId,      AppleColors::Dim);
        setColour (juce::TabbedButtonBar::frontTextColourId,    juce::Colours::white);
        setColour (juce::TabbedButtonBar::tabOutlineColourId,   AppleColors::Line);
        setColour (juce::TabbedButtonBar::frontOutlineColourId, AppleColors::Acc);
    }

    // ---- 圆角按钮（QSS: border-radius:9px; padding:6px 13px）----
    // 注意：被点亮的（toggle 状态）按钮画成蓝色实心 —— 预设按钮 / 标签按钮靠它区分
    void drawButtonBackground (juce::Graphics& g, juce::Button& b,
                               const juce::Colour&,
                               bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const float rad = 9.0f;

        if (b.getToggleState())
        {
            g.setColour (down ? AppleColors::Acc.darker (0.15f)
                              : (over ? juce::Colour (0xFF0A84FF) : AppleColors::Acc));
            g.fillRoundedRectangle (r, rad);
            g.setColour (AppleColors::Acc);
            g.drawRoundedRectangle (r, rad, 1.0f);
            return;
        }

        juce::Colour fill = AppleColors::Card;
        juce::Colour edge = AppleColors::Line;
        if (down)      { fill = juce::Colour (0xFFEAF3FF); edge = AppleColors::Acc; }
        else if (over) { fill = juce::Colours::white;      edge = AppleColors::Acc; }

        g.setColour (fill);
        g.fillRoundedRectangle (r, rad);
        g.setColour (edge);
        g.drawRoundedRectangle (r, rad, 1.0f);
    }

    // ---- 标签页的标签（QSS: 圆角 10px、选中蓝底白字）----
    void drawTabButton (juce::TabBarButton& b, juce::Graphics& g,
                        bool over, bool down) override
    {
        auto area = b.getActiveArea().toFloat();
        if (area.isEmpty()) return;

        const bool front = b.isFrontTab();
        const float rad = 10.0f;

        juce::Path p;
        p.addRoundedRectangle (area.getX(), area.getY(), area.getWidth(), area.getHeight(),
                               rad, rad, true, true, false, false);

        g.setColour (front ? (down ? AppleColors::Acc.darker (0.15f)
                                   : (over ? juce::Colour (0xFF0A84FF) : AppleColors::Acc))
                           : (over || down ? juce::Colour (0xFFE2E2E7) : juce::Colour (0xFFEDEDF0)));
        g.fillPath (p);

        g.setColour (front ? juce::Colours::white : AppleColors::Dim);
        g.setFont (front ? getBoldFont (13.5f) : getAppFont (13.5f));
        g.drawFittedText (b.getButtonText(), b.getTextArea(),
                          juce::Justification::centred, 1);
    }

    // 标签栏底下那条 JUCE 默认的阴影线，我们不要（Python 版靠面板自己的边框）
    void drawTabAreaBehindFrontButton (juce::TabbedButtonBar&, juce::Graphics&,
                                       const int, const int) override {}

    juce::Font getTextButtonFont (juce::TextButton&, int) override
    {
        return getAppFont (13.5f);
    }

    // ---- 下拉框 ----
    void drawComboBox (juce::Graphics& g, int w, int h, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
        const float rad = 8.0f;

        g.setColour (AppleColors::Card);
        g.fillRoundedRectangle (r, rad);
        g.setColour (box.hasKeyboardFocus (false) ? AppleColors::Acc : AppleColors::Line);
        g.drawRoundedRectangle (r, rad, 1.0f);

        // 右侧小三角
        juce::Path p;
        const float cx = (float) w - 13.0f;
        const float cy = (float) h * 0.5f;
        p.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
        g.setColour (AppleColors::Dim);
        g.fillPath (p);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (9, 0, box.getWidth() - 28, box.getHeight());
        label.setFont (getAppFont (13.0f));
    }

    // ---- 滑块（QSS: groove 4px 圆角 + 14px 圆形 handle）----
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                           float sliderPos, float, float,
                           juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        if (style != juce::Slider::LinearHorizontal
            && style != juce::Slider::LinearBar)
            return;

        const float cy = (float) y + (float) h * 0.5f;
        const float x0 = (float) x + 1.0f;
        const float x1 = (float) (x + w) - 1.0f;

        // 轨道底
        g.setColour (AppleColors::Line);
        g.fillRoundedRectangle (x0, cy - 2.0f, x1 - x0, 4.0f, 2.0f);

        // 已填充部分
        const float p = juce::jlimit (x0, x1, sliderPos);
        g.setColour (AppleColors::Acc);
        g.fillRoundedRectangle (x0, cy - 2.0f, p - x0, 4.0f, 2.0f);

        // 圆形手柄
        const float rad = 7.0f;
        g.setColour (juce::Colours::white);
        g.fillEllipse (p - rad, cy - rad, rad * 2.0f, rad * 2.0f);
        g.setColour (juce::Colour (0xFFC9C9CE));
        g.drawEllipse (p - rad, cy - rad, rad * 2.0f, rad * 2.0f, 1.0f);
    }

    // ---- 勾选框（QSS: 16px 圆角 5px 方形）----
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                           bool over, bool) override
    {
        const float size = 16.0f;
        const float rad  = 5.0f;
        auto area = b.getLocalBounds().toFloat();
        const float cy = area.getCentreY();
        juce::Rectangle<float> boxR (area.getX() + 1.0f, cy - size * 0.5f, size, size);

        const bool on = b.getToggleState();
        g.setColour (on ? AppleColors::Acc : juce::Colours::white);
        g.fillRoundedRectangle (boxR, rad);
        g.setColour (on ? AppleColors::Acc : (over ? AppleColors::Acc : AppleColors::Line2));
        g.drawRoundedRectangle (boxR, rad, 1.2f);

        if (on)
        {
            juce::Path tick;
            const float cx = boxR.getCentreX(), c = boxR.getCentreY();
            tick.startNewSubPath (cx - 4.0f, c);
            tick.lineTo (cx - 1.2f, c + 3.0f);
            tick.lineTo (cx + 4.2f, c - 3.2f);
            g.setColour (juce::Colours::white);
            g.strokePath (tick, juce::PathStrokeType (1.9f,
                          juce::PathStrokeType::curved,
                          juce::PathStrokeType::rounded));
        }

        g.setColour (AppleColors::Txt);
        g.setFont (getAppFont (13.0f));
        g.drawText (b.getButtonText(),
                    b.getLocalBounds().withTrimmedLeft ((int) (size + 9.0f)),
                    juce::Justification::centredLeft, false);
    }

    // ---- 滚动条 ----
    void drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h,
                        bool vertical, int thumbPos, int thumbSize,
                        bool, bool) override
    {
        if (thumbSize <= 0) return;
        juce::Rectangle<float> r;
        if (vertical) r = { (float) x + 3.0f, (float) (y + thumbPos), (float) w - 6.0f, (float) thumbSize };
        else          r = { (float) (x + thumbPos), (float) y + 3.0f, (float) thumbSize, (float) h - 6.0f };

        g.setColour (juce::Colour (0xFFC9C9CE));
        g.fillRoundedRectangle (r, r.getWidth() * 0.5f);
    }

    // ---- 全局字体：优先苹方 ----
    static juce::Font getAppFont (float height)
    {
        static juce::String family = pickFamily();
        return juce::Font (juce::FontOptions (family, height, juce::Font::plain));
    }

    static juce::Font getBoldFont (float height)
    {
        static juce::String family = pickFamily();
        return juce::Font (juce::FontOptions (family, height, juce::Font::bold));
    }

private:
    static juce::String pickFamily()
    {
        const juce::StringArray want {
            "PingFang SC",        // macOS 原生苹方
            "MiSans",             // 内嵌苹果风字体
            "HarmonyOS Sans SC",  // 鸿蒙
            "Microsoft YaHei UI",
            "Microsoft YaHei"
        };
        const auto all = juce::Font::findAllTypefaceNames();
        for (const auto& w : want)
            if (all.contains (w, true))
                return w;
        return juce::Font::getDefaultSansSerifFontName();
    }
};

} // namespace miono
