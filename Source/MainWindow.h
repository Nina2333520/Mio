// ============================================================================
//  MioNo - 主窗口（无边框苹果风）
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include "Rack.h"
#include "AppleLookAndFeel.h"
#include "AppleUI.h"

namespace miono
{

class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent()
    {
        setSize (900, 620);
        addAndMakeVisible (title);
        addAndMakeVisible (chain);
        addAndMakeVisible (panel);

        // ---- 设备区 ----
        setupLabel (lblHost,  u8"驱动");
        setupLabel (lblDev,   u8"设备");
        setupLabel (lblRate,  u8"采样率");
        setupLabel (lblBlock, u8"块大小");

        for (auto* c : { &cbHost, &cbDev, &cbRate, &cbBlock })
        {
            c->setJustificationType (juce::Justification::centredLeft);
            addAndMakeVisible (c);
        }

        cbHost.onChange = [this] { refreshDevices(); };
        cbRate.addItem ("48000", 1);
        cbRate.addItem ("44100", 2);
        cbRate.setSelectedId (1);
        cbBlock.addItem ("128",  1);
        cbBlock.addItem ("256",  2);
        cbBlock.addItem ("480",  3);
        cbBlock.addItem ("512",  4);
        cbBlock.setSelectedId (2);

        // ---- 预设 ----
        setupLabel (lblPreset, u8"预设");
        for (int i = 0; i < (int) presets().size(); ++i)
            cbPreset.addItem (presets()[(size_t) i].name, i + 1);
        cbPreset.setSelectedId (2, juce::dontSendNotification);
        cbPreset.onChange = [this]
        {
            const int idx = cbPreset.getSelectedId() - 1;
            rack.applyPreset (idx);
            refreshChain();
        };
        addAndMakeVisible (cbPreset);

        // ---- 按钮 ----
        setupButton (btnStart,   u8"启动", [this] { toggleStart(); });
        setupButton (btnRefresh, u8"刷新", [this] { refreshAll(); });
        setupButton (btnBypass,  u8"旁通", [this]
        {
            rack.setBypass (! rack.getBypass());
            btnBypass.setButtonText (rack.getBypass() ? u8"旁通·开" : u8"旁通");
        });
        setupButton (btnFxOn, u8"启用", [this]
        {
            auto& slots = rack.getSlotsMutable();
            const int r = chain.getSelectedRow();
            if (r >= 0 && r < (int) slots.size())
            {
                slots[(size_t) r].on = ! slots[(size_t) r].on;
                rack.rebuild();
                refreshChain();
            }
        });

        // ---- 电平 ----
        addAndMakeVisible (barIn);
        addAndMakeVisible (barOut);
        lblIn.setText (u8"输入", juce::dontSendNotification);
        lblOut.setText (u8"输出", juce::dontSendNotification);
        for (auto* l : { &lblIn, &lblOut, &lblStat })
        {
            l->setFont (AppleLookAndFeel::getAppFont (12.0f));
            l->setColour (juce::Label::textColourId, AppleColors::Dim);
            addAndMakeVisible (l);
        }
        lblStat.setColour (juce::Label::textColourId, AppleColors::Dim);

        // ---- 联动 ----
        title.onClose    = [this] { if (auto* w = findParentComponentOfClass<juce::DocumentWindow>()) w->closeButtonPressed(); };
        title.onMinimise = [this] { if (auto* w = findParentComponentOfClass<juce::DocumentWindow>()) w->setMinimised (true); };

        chain.onSelect = [this] (int row)
        {
            auto& slots = rack.getSlotsMutable();
            if (row >= 0 && row < (int) slots.size())
                panel.setFx (&slots[(size_t) row]);
            else
                panel.setFx (nullptr);
        };

        panel.onChanged = [this]
        {
            auto& slots = rack.getSlotsMutable();
            const int r = chain.getSelectedRow();
            if (r >= 0 && r < (int) slots.size())
            {
                panel.commit (slots[(size_t) r]);
                rack.rebuild();
            }
        };

        refreshAll();
        startTimerHz (24);
    }

    ~MainComponent() override { rack.stop(); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (AppleColors::Bg);

        // 卡片
        auto card = [&] (juce::Rectangle<int> r)
        {
            g.setColour (AppleColors::Card);
            g.fillRoundedRectangle (r.toFloat(), 12.0f);
            g.setColour (AppleColors::Line.withAlpha (0.85f));
            g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 12.0f, 1.0f);
        };

        card (devArea);
        card (chainArea);
        card (paramArea);
        card (statArea);

        g.setColour (AppleColors::Dim);
        g.setFont (AppleLookAndFeel::getBoldFont (12.5f));
        g.drawText (u8"音频设备",   devArea.getX() + 14, devArea.getY() + 8, 200, 18,
                    juce::Justification::centredLeft, false);
        g.drawText (u8"效果链",     chainArea.getX() + 14, chainArea.getY() + 8, 200, 18,
                    juce::Justification::centredLeft, false);
        g.drawText (u8"参数",       paramArea.getX() + 14, paramArea.getY() + 8, 200, 18,
                    juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        title.setBounds (r.removeFromTop (48));
        r.reduce (12, 8);

        auto top = r.removeFromTop (108);
        devArea = top;
        top.reduce (12, 28);
        layoutDevice (top);

        r.removeFromTop (8);
        auto mid = r.removeFromTop (getHeight() - 48 - 8 - 108 - 8 - 96 - 24);
        auto left = mid.removeFromLeft (int (mid.getWidth() * 0.42));
        chainArea = left;
        left.reduce (12, 30);
        chain.setBounds (left);

        mid.removeFromLeft (8);
        paramArea = mid;
        auto pr = mid.reduced (12, 30);
        panel.setBounds (pr.removeFromTop (juce::jmax (40, pr.getHeight())));

        r.removeFromTop (8);
        statArea = r.removeFromTop (96);
        auto sr = statArea.reduced (12, 30);
        auto row1 = sr.removeFromTop (22);
        lblIn.setBounds (row1.removeFromLeft (38));
        barIn.setBounds (row1.removeFromLeft (juce::jmax (80, row1.getWidth() - 220)));
        row1.removeFromLeft (14);
        lblOut.setBounds (row1.removeFromLeft (38));
        barOut.setBounds (row1);
        sr.removeFromTop (4);
        lblStat.setBounds (sr.removeFromTop (22));
    }

private:
    void setupLabel (juce::Label& l, const juce::String& t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setFont (AppleLookAndFeel::getAppFont (12.5f));
        l.setColour (juce::Label::textColourId, AppleColors::Dim);
        l.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (l);
    }

    void setupButton (juce::TextButton& b, const juce::String& t, std::function<void()> fn)
    {
        b.setButtonText (t);
        b.onClick = std::move (fn);
        addAndMakeVisible (b);
    }

    void layoutDevice (juce::Rectangle<int> a)
    {
        const int labelW = 62;
        const int rowH = 24, gap = 4;

        auto row1 = a.removeFromTop (rowH); a.removeFromTop (gap);
        auto row2 = a.removeFromTop (rowH); a.removeFromTop (gap);
        auto row3 = a.removeFromTop (rowH);

        // 第一行：驱动 + 设备
        lblHost.setBounds (row1.removeFromLeft (labelW));
        cbHost.setBounds (row1.removeFromLeft (juce::jmax (90, (row1.getWidth() - 220) / 2)));
        row1.removeFromLeft (10);
        lblDev.setBounds (row1.removeFromLeft (labelW));
        cbDev.setBounds (row1);

        // 第二行：采样率 + 块大小 + 预设
        lblRate.setBounds (row2.removeFromLeft (labelW));
        cbRate.setBounds (row2.removeFromLeft (90));
        row2.removeFromLeft (12);
        lblBlock.setBounds (row2.removeFromLeft (labelW));
        cbBlock.setBounds (row2.removeFromLeft (90));
        row2.removeFromLeft (12);
        lblPreset.setBounds (row2.removeFromLeft (labelW));
        cbPreset.setBounds (juce::jmax (100, row2.getWidth() - 250));

        // 第三行：按钮
        btnStart.setBounds   (row3.removeFromLeft (78));  row3.removeFromLeft (7);
        btnRefresh.setBounds (row3.removeFromLeft (78));  row3.removeFromLeft (7);
        btnBypass.setBounds  (row3.removeFromLeft (84));  row3.removeFromLeft (7);
        btnFxOn.setBounds    (row3.removeFromLeft (78));
    }

    void refreshAll()
    {
        cbHost.clear (juce::dontSendNotification);
        auto hosts = Rack::listHostTypes();
        for (int i = 0; i < hosts.size(); ++i)
            cbHost.addItem (hosts[i], i + 1);
        // 优先 ASIO
        int asioId = 0;
        for (int i = 0; i < hosts.size(); ++i)
            if (hosts[i].containsIgnoreCase ("asio")) { asioId = i + 1; break; }
        cbHost.setSelectedId (asioId > 0 ? asioId : (hosts.isEmpty() ? 0 : 1),
                              juce::dontSendNotification);
        refreshDevices();
        refreshChain();
    }

    void refreshDevices()
    {
        cbDev.clear (juce::dontSendNotification);
        const auto host = cbHost.getText();
        auto devs = Rack::listDevices (host);
        for (int i = 0; i < devs.size(); ++i)
            cbDev.addItem (devs[i], i + 1);
        if (! devs.isEmpty())
            cbDev.setSelectedId (1, juce::dontSendNotification);
    }

    void refreshChain()
    {
        chain.setSlots (&rack.getSlots());
        const int r = chain.getSelectedRow();
        auto& slots = rack.getSlotsMutable();
        if (r >= 0 && r < (int) slots.size())
            panel.setFx (&slots[(size_t) r]);
        repaint();
    }

    void toggleStart()
    {
        if (rack.isRunning())
        {
            rack.stop();
            btnStart.setButtonText (u8"启动");
        }
        else
        {
            const double sr = cbRate.getText().getDoubleValue();
            const int    bs = cbBlock.getText().getIntValue();
            const auto err = rack.start (cbHost.getText(), cbDev.getText(), sr, bs);
            if (err.isNotEmpty())
            {
                juce::NativeMessageBox::showMessageBoxAsync (
                    juce::MessageBoxIconType::WarningIcon,
                    "MioNo", u8"开流失败：\n" + err);
                btnStart.setButtonText (u8"启动");
                return;
            }
            btnStart.setButtonText (u8"停止");
        }
        title.refresh();
    }

    void timerCallback() override
    {
        barIn.setLevel (rack.getInLevel());
        barOut.setLevel (rack.getOutLevel());
        barIn.setClip (rack.getClipping());
        barOut.setClip (rack.getClipping());

        juce::String s;
        if (rack.isRunning())
            s = rack.deviceInfo()
              + juce::String ("   单块 ") + juce::String (rack.getBlockMs(), 2) + " ms";
        else
            s = u8"未开流 · 选择设备后点「启动」";
        if (lblStat.getText() != s) lblStat.setText (s, juce::dontSendNotification);

        title.refresh();
    }

    Rack rack;
    TitleBar title { rack };
    ChainList chain;
    FxParamPanel panel;

    juce::Label lblHost, lblDev, lblRate, lblBlock, lblPreset;
    juce::ComboBox cbHost, cbDev, cbRate, cbBlock, cbPreset;
    juce::TextButton btnStart, btnRefresh, btnBypass, btnFxOn;

    juce::Label lblIn, lblOut, lblStat;
    LevelBar barIn, barOut;

    juce::Rectangle<int> devArea, chainArea, paramArea, statArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

// ---------------------------------------------------------------------------
// 无边框主窗口
// ---------------------------------------------------------------------------
class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow (juce::String name)
        : juce::DocumentWindow (name, AppleColors::Bg, juce::DocumentWindow::allButtons)
    {
        // 关闭系统标题栏，标题栏由 TitleBar 组件自绘
        setUsingNativeTitleBar (false);
        setTitleBarButtonsRequired (0, false);
        setContentOwned (new MainComponent(), true);
        setResizable (true, false);
        setResizeLimits (780, 540, 2200, 1600);
        centreWithSize (900, 620);
        setVisible (true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};

} // namespace miono
