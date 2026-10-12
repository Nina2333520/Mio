// ============================================================================
//  MioNo - 主界面（苹果风无边框窗口）
//  对应 Python 版 MioNo.py 的 TitleBar + Win + ChainEditor
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include "Rack.h"
#include "AppleLookAndFeel.h"

namespace miono
{

// ---------------------------------------------------------------------------
// 电平条 —— 自绘圆角渐变，对应 Python 版 LevelBar
// ---------------------------------------------------------------------------
class LevelBar : public juce::Component
{
public:
    void setLevel (float v) { level = juce::jlimit (0.0f, 1.0f, v); repaint(); }
    void setClip (bool c)   { if (clip != c) { clip = c; repaint(); } }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (AppleColors::Line);
        g.fillRoundedRectangle (r, 4.0f);

        if (level > 0.005f)
        {
            // dB 感知：非线性映射，低电平也看得见
            const float db = juce::Decibels::gainToDecibels (level, -60.0f);
            const float t  = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
            const float w  = r.getWidth() * t;

            juce::ColourGradient grad (AppleColors::Green, r.getX(), 0.0f,
                                       AppleColors::Red,   r.getRight(), 0.0f, false);
            grad.addColour (0.72, AppleColors::Orange);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (r.withWidth (juce::jmax (4.0f, w)), 4.0f);
        }

        if (clip)
        {
            g.setColour (AppleColors::Red);
            g.fillRoundedRectangle (r.getRight() - 4.0f, r.getY(), 4.0f, r.getHeight(), 2.0f);
        }
    }

private:
    float level = 0.0f;
    bool  clip  = false;
};

// ---------------------------------------------------------------------------
// 状态小灯 —— 对应 Python 版 Lamp
// ---------------------------------------------------------------------------
class Lamp : public juce::Component
{
public:
    Lamp (juce::String t, juce::Colour c) : text (std::move (t)), colour (c) {}
    void setOn (bool b) { if (on != b) { on = b; repaint(); } }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const float d = juce::jmin (9.0f, r.getHeight() - 4.0f);
        auto dot = juce::Rectangle<float> (r.getX() + 1.0f,
                                           r.getCentreY() - d * 0.5f, d, d);
        g.setColour (on ? colour : AppleColors::Line2);
        g.fillEllipse (dot);

        g.setColour (on ? AppleColors::Txt : AppleColors::Dim);
        g.setFont (AppleLookAndFeel::getAppFont (12.0f));
        g.drawText (text, getLocalBounds().withTrimmedLeft ((int) (d + 6.0f)),
                    juce::Justification::centredLeft, false);
    }

private:
    juce::String text;
    juce::Colour colour;
    bool on = false;
};

// ---------------------------------------------------------------------------
// 单个效果的参数面板
// ---------------------------------------------------------------------------
// 每个效果有哪些旋钮（与 FxEngine 的 configure 对应）
struct KnobDef
{
    juce::String id;
    juce::String label;
    float minVal, maxVal, defVal;
    juce::String suffix;
};

inline std::vector<KnobDef> knobsFor (FxType t)
{
    switch (t)
    {
        case FxType::SpectralDenoise:
            return { { "strength", u8"强度", 0.0f, 1.0f, 0.5f, "" } };
        case FxType::NoiseGate:
            return { { "threshold", u8"阈值", -80.0f, 0.0f, -45.0f, " dB" },
                     { "attack",    u8"起音", 0.1f, 50.0f, 3.0f, " ms" },
                     { "release",   u8"释放", 10.0f, 800.0f, 120.0f, " ms" } };
        case FxType::Highpass:
            return { { "freq", u8"频率", 20.0f, 1000.0f, 90.0f, " Hz" },
                     { "q",    u8"Q",    0.1f, 4.0f, 0.707f, "" } };
        case FxType::Lowpass:
            return { { "freq", u8"频率", 1000.0f, 20000.0f, 12000.0f, " Hz" },
                     { "q",    u8"Q",    0.1f, 4.0f, 0.707f, "" } };
        case FxType::PeakEQ:
            return { { "freq", u8"频率", 20.0f, 20000.0f, 1000.0f, " Hz" },
                     { "q",    u8"Q",    0.1f, 8.0f, 1.0f, "" },
                     { "gain", u8"增益", -18.0f, 18.0f, 0.0f, " dB" } };
        case FxType::Compressor:
            return { { "threshold", u8"阈值", -60.0f, 0.0f, -18.0f, " dB" },
                     { "ratio",     u8"比率", 1.0f, 20.0f, 3.0f, ":1" },
                     { "attack",    u8"起音", 0.1f, 200.0f, 10.0f, " ms" },
                     { "release",   u8"释放", 10.0f, 1000.0f, 100.0f, " ms" },
                     { "knee",      u8"拐点", 0.0f, 24.0f, 6.0f, " dB" } };
        case FxType::Reverb:
            return { { "size", u8"空间", 0.0f, 1.0f, 0.5f, "" },
                     { "damp", u8"阻尼", 0.0f, 1.0f, 0.5f, "" },
                     { "wet",  u8"湿度", -40.0f, 0.0f, -6.0f, " dB" } };
        case FxType::Delay:
            return { { "time",     u8"时间", 20.0f, 1200.0f, 250.0f, " ms" },
                     { "feedback", u8"反馈", 0.0f, 0.85f, 0.3f, "" },
                     { "wet",      u8"湿度", -40.0f, 0.0f, -10.0f, " dB" } };
        case FxType::Octave:
            return { { "mix", u8"混合", -40.0f, 0.0f, -12.0f, " dB" } };
        case FxType::Limiter:
            return { { "ceiling", u8"上限", -12.0f, 0.0f, -1.0f, " dB" },
                     { "release", u8"释放", 10.0f, 500.0f, 60.0f, " ms" } };
        default: return {};
    }
}

class FxParamPanel : public juce::Component
{
public:
    std::function<void()> onChanged;

    FxParamPanel()
    {
        for (auto* s : { &sStrength, &sA, &sB, &sC, &sD })
        {
            s->setSliderStyle (juce::Slider::LinearHorizontal);
            s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 62, 20);
            s->setColour (juce::Slider::thumbColourId, AppleColors::Acc);
            s->onValueChange = [this] { if (onChanged) onChanged(); };
            addChildComponent (s);
        }
    }

    void setFx (const FxSlot* slot)
    {
        current = slot;
        for (auto* s : { &sStrength, &sA, &sB, &sC, &sD })
            s->setVisible (false);

        if (slot == nullptr) { repaint(); return; }

        auto defs = knobsFor (slot->type);
        juce::Slider* arr[5] = { &sStrength, &sA, &sB, &sC, &sD };
        for (size_t i = 0; i < defs.size() && i < 5; ++i)
        {
            auto& d = defs[i];
            auto* s = arr[i];
            s->setRange ((double) d.minVal, (double) d.maxVal, 0.001);
            s->setTextValueSuffix (d.suffix);
            s->setValue ((double) slot->get (d.id, d.defVal), juce::dontSendNotification);
            s->setVisible (true);
        }
        resized();
        repaint();
    }

    // 把界面上的值写回 slot
    void commit (FxSlot& slot) const
    {
        auto defs = knobsFor (slot.type);
        const juce::Slider* arr[5] = { &sStrength, &sA, &sB, &sC, &sD };
        for (size_t i = 0; i < defs.size() && i < 5; ++i)
            slot.set (defs[i].id, (float) arr[i]->getValue());
        juce::ignoreUnused (slot);
    }

    void resized() override
    {
        if (current == nullptr) return;
        auto defs = knobsFor (current->type);
        const int rowH = 34;
        int y = 4;
        juce::Slider* arr[5] = { &sStrength, &sA, &sB, &sC, &sD };
        for (size_t i = 0; i < defs.size() && i < 5; ++i)
        {
            arr[i]->setBounds (118, y, getWidth() - 126, rowH - 4);
            y += rowH;
        }
    }

    void paint (juce::Graphics& g) override
    {
        if (current == nullptr) return;
        auto defs = knobsFor (current->type);
        const int rowH = 34;
        int y = 4;
        g.setFont (AppleLookAndFeel::getAppFont (12.5f));
        for (size_t i = 0; i < defs.size() && i < 5; ++i)
        {
            g.setColour (AppleColors::Dim);
            g.drawText (defs[i].label, 8, y, 105, rowH - 4,
                        juce::Justification::centredLeft, false);
            y += rowH;
        }
    }

private:
    const FxSlot* current = nullptr;
    juce::Slider sStrength, sA, sB, sC, sD;
};

// ---------------------------------------------------------------------------
// 效果链列表（左）
// ---------------------------------------------------------------------------
class ChainList : public juce::Component,
                  public juce::ListBoxModel
{
public:
    std::function<void (int)> onSelect;

    ChainList()
    {
        list.setModel (this);
        list.setRowHeight (30);
        list.setColour (juce::ListBox::backgroundColourId, AppleColors::Card);
        list.setOutlineThickness (0);
        addAndMakeVisible (list);
    }

    void setSlots (const std::vector<FxSlot>* s)
    {
        slots = s;
        list.updateContent();
        if (slots != nullptr && ! slots->empty())
            list.selectRow (selectedRow < (int) slots->size() ? selectedRow : 0);
        repaint();
    }

    int getSelectedRow() const { return selectedRow; }

    void resized() override { list.setBounds (getLocalBounds().reduced (1)); }

    int getNumRows() override
    {
        return slots == nullptr ? 0 : (int) slots->size();
    }

    void paintListBoxItem (int row, juce::Graphics& g,
                           int width, int height, bool selected) override
    {
        if (slots == nullptr || row < 0 || row >= (int) slots->size()) return;
        const auto& s = (*slots)[(size_t) row];

        auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (2.0f, 1.0f);
        if (selected)
        {
            g.setColour (AppleColors::Acc.withAlpha (0.12f));
            g.fillRoundedRectangle (r, 7.0f);
            g.setColour (AppleColors::Acc.withAlpha (0.35f));
            g.drawRoundedRectangle (r, 7.0f, 1.0f);
        }

        // 开关圆点
        const float d = 8.0f;
        auto dot = juce::Rectangle<float> (9.0f, (float) height * 0.5f - d * 0.5f, d, d);
        g.setColour (s.on ? AppleColors::Green : AppleColors::Line2);
        g.fillEllipse (dot);

        g.setColour (selected ? AppleColors::Acc : AppleColors::Txt);
        g.setFont (AppleLookAndFeel::getAppFont (13.0f));
        g.drawText (juce::String (s.on ? "" : "") + fxName (s.type),
                    24, 0, width - 30, height,
                    juce::Justification::centredLeft, false);

        // 序号
        g.setColour (AppleColors::Dim);
        g.setFont (AppleLookAndFeel::getAppFont (11.0f));
        g.drawText (juce::String (row + 1), width - 24, 0, 18, height,
                    juce::Justification::centredRight, false);
    }

    void listBoxItemClicked (int row, const juce::MouseEvent&) override
    {
        selectedRow = row;
        if (onSelect) onSelect (row);
    }

private:
    juce::ListBox list;
    const std::vector<FxSlot>* slots = nullptr;
    int selectedRow = 0;
};

// ---------------------------------------------------------------------------
// 自绘标题栏 —— 对应 Python 版 TitleBar（红黄绿圆点 + 头像 + 标题 + 状态灯）
// ---------------------------------------------------------------------------
class TitleBar : public juce::Component
{
public:
    TitleBar (Rack& r) : rack (r)
    {
        addAndMakeVisible (lampAsio);
        addAndMakeVisible (lampRun);
    }

    std::function<void()> onClose, onMinimise;

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();

        // 顶部圆角（继承窗口的圆角）
        juce::Path p;
        const float rad = 13.0f;
        p.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight() + rad,
                               rad, rad, true, true, false, false);
        g.setColour (AppleColors::Card2);
        g.fillPath (p);

        g.setColour (AppleColors::Line);
        g.drawLine (0.0f, r.getBottom() - 0.5f, r.getWidth(), r.getBottom() - 0.5f, 1.0f);

        // 红黄绿圆点
        const float d = 12.0f, gap = 7.0f;
        float x = 15.0f;
        const float cy = r.getHeight() * 0.5f;
        drawDot (g, x, cy, d, AppleColors::Red);        x += d + gap;
        drawDot (g, x, cy, d, AppleColors::Orange);     x += d + gap;
        drawDot (g, x, cy, d, AppleColors::Green);

        // 头像
        const float ax = 15.0f + d * 3.0f + gap * 2.0f + 14.0f;
        const float asz = 26.0f;
        juce::Rectangle<float> av (ax, cy - asz * 0.5f, asz, asz);
        g.setColour (AppleColors::Acc.withAlpha (0.16f));
        g.fillEllipse (av);
        g.setColour (AppleColors::Acc);
        g.setFont (AppleLookAndFeel::getBoldFont (13.0f));
        g.drawText ("M", av, juce::Justification::centred, false);

        // 标题
        g.setColour (AppleColors::Txt);
        g.setFont (AppleLookAndFeel::getBoldFont (14.5f));
        g.drawText (juce::String (APP_NAME) + " " + VERSION,
                    (int) (av.getRight() + 10.0f), 0, 200, getHeight(),
                    juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        lampAsio.setBounds (r.getWidth() - 190, 0, 78, r.getHeight());
        lampRun .setBounds (r.getWidth() - 108, 0, 92, r.getHeight());
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        // 红黄绿点击
        const float d = 12.0f, gap = 7.0f;
        float x = 15.0f;
        for (int i = 0; i < 3; ++i)
        {
            if (e.x >= x - 3 && e.x <= x + d + 3)
            {
                if (i == 0 && onClose)    { onClose();    return; }
                if (i == 1 && onMinimise) { onMinimise(); return; }
                return;
            }
            x += d + gap;
        }
        // 其余区域拖动窗口
        dragStart = e.getScreenPosition();
        if (auto* w = getTopLevelComponent())
            if (auto* dw = dynamic_cast<juce::DocumentWindow*> (w))
                dragWinStart = dw->getScreenBounds().getTopLeft();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragStart.isOrigin()) return;
        if (auto* w = getTopLevelComponent())
            if (auto* dw = dynamic_cast<juce::DocumentWindow*> (w))
            {
                const auto delta = e.getScreenPosition() - dragStart;
                dw->setTopLeftPosition (dragWinStart.x + delta.x,
                                        dragWinStart.y + delta.y);
            }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        dragStart = {};
        dragWinStart = {};
    }

    void refresh()
    {
        lampAsio.setOn (rack.getBlockMs() >= 0.0 && rack.isRunning());
        lampRun .setOn (rack.isRunning());
    }

    static constexpr const char* APP_NAME = "MioNo";
    static constexpr const char* VERSION  = "v6.0";

private:
    static void drawDot (juce::Graphics& g, float x, float cy, float d, juce::Colour c)
    {
        g.setColour (c);
        g.fillEllipse (x, cy - d * 0.5f, d, d);
    }

    Rack& rack;
    Lamp lampAsio { u8"ASIO", AppleColors::Acc };
    Lamp lampRun  { u8"运行", AppleColors::Green };
    juce::Point<int> dragStart, dragWinStart;
};

} // namespace miono
