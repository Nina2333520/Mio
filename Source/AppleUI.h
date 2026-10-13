// ============================================================================
//  MioNo - 主界面组件（苹果风无边框窗口）
//  v6.2：电平条 Peak+RMS、效果链可增删上下移、标题栏绿点最大化、可缩到状态栏
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include "Rack.h"
#include "AppleLookAndFeel.h"

namespace miono
{

// ---------------------------------------------------------------------------
// 电平条 —— 上面一条 Peak（绿→红），下面一条 RMS（蓝），右端 CLIP 红块
// ---------------------------------------------------------------------------
class LevelBar : public juce::Component
{
public:
    void setLevels (float peakLevel, float rmsLevel)
    {
        const float p = juce::jlimit (0.0f, 1.0f, peakLevel);
        const float r = juce::jlimit (0.0f, 1.0f, rmsLevel);
        if (std::abs (p - peak) > 0.001f || std::abs (r - rms) > 0.001f)
        {
            peak = p;
            rms  = r;
            repaint();
        }
    }

    void setClip (bool c) { if (clip != c) { clip = c; repaint(); } }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();

        g.setColour (AppleColors::Line);
        g.fillRoundedRectangle (r, 4.0f);

        const float barH = juce::jmax (2.0f, (r.getHeight() - 3.0f) * 0.5f);
        const float topY = r.getY() + 1.0f;
        const float botY = topY + barH + 1.0f;

        // Peak
        if (peak > 0.005f)
        {
            const float db = juce::Decibels::gainToDecibels (peak, -60.0f);
            const float t  = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
            const float w  = juce::jmax (3.0f, r.getWidth() * t);

            juce::ColourGradient grad (AppleColors::Green, r.getX(), 0.0f,
                                       AppleColors::Red,   r.getRight(), 0.0f, false);
            grad.addColour (0.72, AppleColors::Orange);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (r.getX(), topY, w, barH, 2.0f);
        }

        // RMS
        if (rms > 0.005f)
        {
            const float db = juce::Decibels::gainToDecibels (rms, -60.0f);
            const float t  = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
            const float w  = juce::jmax (3.0f, r.getWidth() * t);

            g.setColour (AppleColors::Acc);
            g.fillRoundedRectangle (r.getX(), botY, w, barH, 2.0f);
        }

        if (clip)
        {
            g.setColour (AppleColors::Red);
            g.fillRoundedRectangle (r.getRight() - 4.0f, r.getY(), 4.0f, r.getHeight(), 2.0f);
        }
    }

private:
    float peak = 0.0f, rms = 0.0f;
    bool  clip = false;
};

// ---------------------------------------------------------------------------
// 状态小灯
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
            return { { "strength", L"强度", 0.0f, 1.0f, 0.5f, "" } };
        case FxType::NoiseGate:
            return { { "threshold", L"阈值", -80.0f, 0.0f, -45.0f, " dB" },
                     { "attack",    L"起音", 0.1f, 50.0f, 3.0f, " ms" },
                     { "release",   L"释放", 10.0f, 800.0f, 120.0f, " ms" } };
        case FxType::Highpass:
            return { { "freq", L"频率", 20.0f, 1000.0f, 90.0f, " Hz" },
                     { "q",    L"Q",    0.1f, 4.0f, 0.707f, "" } };
        case FxType::Lowpass:
            return { { "freq", L"频率", 1000.0f, 20000.0f, 12000.0f, " Hz" },
                     { "q",    L"Q",    0.1f, 4.0f, 0.707f, "" } };
        case FxType::PeakEQ:
            return { { "freq", L"频率", 20.0f, 20000.0f, 1000.0f, " Hz" },
                     { "q",    L"Q",    0.1f, 8.0f, 1.0f, "" },
                     { "gain", L"增益", -18.0f, 18.0f, 0.0f, " dB" } };
        case FxType::Compressor:
            return { { "threshold", L"阈值", -60.0f, 0.0f, -18.0f, " dB" },
                     { "ratio",     L"比率", 1.0f, 20.0f, 3.0f, ":1" },
                     { "attack",    L"起音", 0.1f, 200.0f, 10.0f, " ms" },
                     { "release",   L"释放", 10.0f, 1000.0f, 100.0f, " ms" },
                     { "knee",      L"拐点", 0.0f, 24.0f, 6.0f, " dB" } };
        case FxType::Reverb:
            return { { "size", L"空间", 0.0f, 1.0f, 0.5f, "" },
                     { "damp", L"阻尼", 0.0f, 1.0f, 0.5f, "" },
                     { "wet",  L"湿度", -40.0f, 0.0f, -6.0f, " dB" } };
        case FxType::Delay:
            return { { "time",     L"时间", 20.0f, 1200.0f, 250.0f, " ms" },
                     { "feedback", L"反馈", 0.0f, 0.85f, 0.3f, "" },
                     { "wet",      L"湿度", -40.0f, 0.0f, -10.0f, " dB" } };
        case FxType::Octave:
            return { { "mix", L"混合", -40.0f, 0.0f, -12.0f, " dB" } };
        case FxType::Limiter:
            return { { "ceiling", L"上限", -12.0f, 0.0f, -1.0f, " dB" },
                     { "release", L"释放", 10.0f, 500.0f, 60.0f, " ms" } };
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

    void commit (FxSlot& slot) const
    {
        auto defs = knobsFor (slot.type);
        const juce::Slider* arr[5] = { &sStrength, &sA, &sB, &sC, &sD };
        for (size_t i = 0; i < defs.size() && i < 5; ++i)
            slot.set (defs[i].id, (float) arr[i]->getValue());
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
// 效果链编辑器（类型下拉 + 列表 + ＋ / ↑ / ↓ / 🗑）
// ---------------------------------------------------------------------------
class ChainList : public juce::Component,
                  public juce::ListBoxModel
{
public:
    std::function<void (int)>     onSelect;
    std::function<void (FxType)>  onAdd;
    std::function<void()>         onMoveUp, onMoveDown, onDelete;

    ChainList()
    {
        list.setModel (this);
        list.setRowHeight (28);
        list.setColour (juce::ListBox::backgroundColourId, AppleColors::Card);
        list.setOutlineThickness (0);
        addAndMakeVisible (list);

        for (int i = 0; i < fxTypeCount(); ++i)
            kind.addItem (fxName (fxTypeFromIndex (i)), i + 1);
        kind.setSelectedId (1, juce::dontSendNotification);
        addAndMakeVisible (kind);

        auto setup = [this] (juce::TextButton& b, const juce::String& t, std::function<void()> fn)
        {
            b.setButtonText (t);
            b.onClick = std::move (fn);
            addAndMakeVisible (b);
        };
        setup (btnAdd,  L"＋", [this] { if (onAdd) onAdd (fxTypeFromIndex (kind.getSelectedId() - 1)); });
        setup (btnUp,   L"↑", [this] { if (onMoveUp) onMoveUp(); });
        setup (btnDown, L"↓", [this] { if (onMoveDown) onMoveDown(); });
        setup (btnDel,  L"🗑", [this] { if (onDelete) onDelete(); });
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
    void setSelectedRow (int row) { selectedRow = row; }

    void resized() override
    {
        auto r = getLocalBounds();
        auto top = r.removeFromTop (28);
        kind.setBounds (top.removeFromLeft (juce::jmax (80, top.getWidth() - 128)));
        top.removeFromLeft (4);
        btnAdd .setBounds (top.removeFromLeft (28));
        btnUp  .setBounds (top.removeFromLeft (28));
        btnDown.setBounds (top.removeFromLeft (28));
        btnDel .setBounds (top.removeFromLeft (28));
        r.removeFromTop (4);
        list.setBounds (r.reduced (1));
    }

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

        const float d = 8.0f;
        auto dot = juce::Rectangle<float> (9.0f, (float) height * 0.5f - d * 0.5f, d, d);
        g.setColour (s.on ? AppleColors::Green : AppleColors::Line2);
        g.fillEllipse (dot);

        g.setColour (selected ? AppleColors::Acc : AppleColors::Txt);
        g.setFont (AppleLookAndFeel::getAppFont (13.0f));
        g.drawText (fxName (s.type), 24, 0, width - 46, height,
                    juce::Justification::centredLeft, false);

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
    juce::ListBox    list;
    juce::ComboBox   kind;
    juce::TextButton btnAdd, btnUp, btnDown, btnDel;
    const std::vector<FxSlot>* slots = nullptr;
    int selectedRow = 0;
};

// ---------------------------------------------------------------------------
// 自绘标题栏（红黄绿圆点 + 头像 + 标题 + 状态灯）
// ---------------------------------------------------------------------------
class TitleBar : public juce::Component
{
public:
    TitleBar (Rack& r) : rack (r)
    {
        addAndMakeVisible (lampAsio);
        addAndMakeVisible (lampRun);
    }

    std::function<void()> onClose, onMinimise, onMaximise;

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();

        juce::Path p;
        const float rad = 13.0f;
        p.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight() + rad,
                               rad, rad, true, true, false, false);
        g.setColour (AppleColors::Card2);
        g.fillPath (p);

        g.setColour (AppleColors::Line);
        g.drawLine (0.0f, r.getBottom() - 0.5f, r.getWidth(), r.getBottom() - 0.5f, 1.0f);

        const float d = 12.0f, gap = 7.0f;
        float x = 15.0f;
        const float cy = r.getHeight() * 0.5f;
        drawDot (g, x, cy, d, AppleColors::Red);        x += d + gap;
        drawDot (g, x, cy, d, AppleColors::Orange);     x += d + gap;
        drawDot (g, x, cy, d, AppleColors::Green);

        const float ax = 15.0f + d * 3.0f + gap * 2.0f + 14.0f;
        const float asz = 26.0f;
        juce::Rectangle<float> av (ax, cy - asz * 0.5f, asz, asz);
        g.setColour (AppleColors::Acc.withAlpha (0.16f));
        g.fillEllipse (av);
        g.setColour (AppleColors::Acc);
        g.setFont (AppleLookAndFeel::getBoldFont (13.0f));
        g.drawText ("M", av, juce::Justification::centred, false);

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
        const float d = 12.0f, gap = 7.0f;
        float x = 15.0f;
        for (int i = 0; i < 3; ++i)
        {
            if (e.x >= x - 3 && e.x <= x + d + 3)
            {
                if (i == 0 && onClose)    { onClose();    return; }
                if (i == 1 && onMinimise) { onMinimise(); return; }
                if (i == 2 && onMaximise) { onMaximise(); return; }
                return;
            }
            x += d + gap;
        }

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

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        if (onMaximise) onMaximise();
    }

    void refresh()
    {
        lampAsio.setOn (rack.isRunning());
        lampRun .setOn (rack.isRunning());
    }

    static constexpr const char* APP_NAME = "MioNo";
    static constexpr const char* VERSION  = "v6.2";

private:
    static void drawDot (juce::Graphics& g, float x, float cy, float d, juce::Colour c)
    {
        g.setColour (c);
        g.fillEllipse (x, cy - d * 0.5f, d, d);
    }

    Rack& rack;
    Lamp lampAsio { L"ASIO", AppleColors::Acc };
    Lamp lampRun  { L"运行", AppleColors::Green };
    juce::Point<int> dragStart, dragWinStart;
};

} // namespace miono
