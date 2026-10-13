// ============================================================================
//  MioNo - 主窗口（无边框苹果风）
//  v6.1：输入/输出分开选、增益/声像、空灵层、配置存盘、开机自启、Peak+RMS
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
        setSize (1000, 700);
        addAndMakeVisible (title);
        addAndMakeVisible (chain);
        addAndMakeVisible (panel);

        // ---------------- 设备区 ----------------
        setupLabel (lblHost,   L"驱动");
        setupLabel (lblInDev,  L"输入");
        setupLabel (lblOutDev, L"输出");
        setupLabel (lblRate,   L"采样率");
        setupLabel (lblBlock,  L"块大小");
        setupLabel (lblPreset, L"预设");

        for (auto* c : { &cbHost, &cbIn, &cbOut, &cbRate, &cbBlock, &cbPreset })
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

        for (int i = 0; i < (int) presets().size(); ++i)
            cbPreset.addItem (presets()[(size_t) i].name, i + 1);
        cbPreset.setSelectedId (2, juce::dontSendNotification);
        cbPreset.onChange = [this]
        {
            rack.applyPreset (cbPreset.getSelectedId() - 1);
            refreshChain();
            refreshLayerControls();
        };
        addAndMakeVisible (cbPreset);

        // ---------------- 增益 / 声像 ----------------
        setupLabel (lblGainIn,  L"输入增益");
        setupLabel (lblGainOut, L"输出增益");
        setupLabel (lblPan,     L"声像");
        setupGainSlider (slGainIn,  -24.0, 24.0, 0.0, " dB");
        setupGainSlider (slGainOut, -24.0, 24.0, 0.0, " dB");
        setupGainSlider (slPan,      -1.0,  1.0, 0.0, "");

        slGainIn .onValueChange = [this] { rack.setInputGainDb  ((float) slGainIn .getValue()); };
        slGainOut.onValueChange = [this] { rack.setOutputGainDb ((float) slGainOut.getValue()); };
        slPan    .onValueChange = [this] { rack.setPan          ((float) slPan    .getValue()); };

        // ---------------- 按钮 ----------------
        setupButton (btnStart,   L"▶ 启动", [this] { toggleStart(); });
        setupButton (btnRefresh, L"刷新",   [this] { refreshAll(); });
        setupButton (btnBypass,  L"旁通",   [this]
        {
            rack.setBypass (! rack.getBypass());
            btnBypass.setButtonText (rack.getBypass() ? L"旁通·开" : L"旁通");
        });
        setupButton (btnFxOn, L"启用/旁通", [this]
        {
            auto& slots = activeSlots();
            const int r = chain.getSelectedRow();
            if (r >= 0 && r < (int) slots.size())
            {
                slots[(size_t) r].on = ! slots[(size_t) r].on;
                rack.rebuild();
                refreshChain();
            }
        });
        setupButton (btnSave, L"💾 保存", [this] { saveConfig(); });
        setupButton (btnLoad, L"📂 载入", [this] { loadConfig(); });

        tgAuto.setButtonText (L"开机自启");
        tgAuto.setToggleState (Rack::getAutoStart(), juce::dontSendNotification);
        tgAuto.onClick = [this] { Rack::setAutoStart (tgAuto.getToggleState()); };
        addAndMakeVisible (tgAuto);

        // ---------------- 两条链的切换 ----------------
        setupButton (btnTabMain,  L"🎤 主效果链", [this] { setEditingLayer (false); });
        setupButton (btnTabLayer, L"✨ 空灵层",   [this] { setEditingLayer (true); });
        btnTabMain .setClickingTogglesState (true);
        btnTabLayer.setClickingTogglesState (true);
        btnTabMain .setRadioGroupId (1001, juce::dontSendNotification);
        btnTabLayer.setRadioGroupId (1001, juce::dontSendNotification);
        btnTabMain .setToggleState (true, juce::dontSendNotification);

        tgLayer.setButtonText (L"空灵层开");
        tgLayer.onClick = [this] { rack.setLayerOn (tgLayer.getToggleState()); };
        addAndMakeVisible (tgLayer);

        setupLabel (lblLayerGain, L"层音量");
        setupGainSlider (slLayerGain, -40.0, 6.0, -8.0, " dB");
        slLayerGain.onValueChange = [this] { rack.setLayerGainDb ((float) slLayerGain.getValue()); };

        // ---------------- 电平 ----------------
        addAndMakeVisible (barIn);
        addAndMakeVisible (barOut);
        addAndMakeVisible (lampClip);
        lblIn.setText (L"输入", juce::dontSendNotification);
        lblOut.setText (L"输出", juce::dontSendNotification);
        for (auto* l : { &lblIn, &lblOut, &lblStat })
        {
            l->setFont (AppleLookAndFeel::getAppFont (12.0f));
            l->setColour (juce::Label::textColourId, AppleColors::Dim);
            addAndMakeVisible (l);
        }
        lblStat.setColour (juce::Label::textColourId, AppleColors::Dim);

        // ---------------- 联动 ----------------
        title.onClose    = [this] { if (auto* w = findParentComponentOfClass<juce::DocumentWindow>()) w->closeButtonPressed(); };
        title.onMinimise = [this] { if (auto* w = findParentComponentOfClass<juce::DocumentWindow>()) w->setMinimised (true); };
        title.onMaximise = [this] { toggleMaximise(); };

        chain.onSelect = [this] (int row)
        {
            auto& slots = activeSlots();
            if (row >= 0 && row < (int) slots.size())
                panel.setFx (&slots[(size_t) row]);
            else
                panel.setFx (nullptr);
        };

        chain.onAdd = [this] (FxType t)
        {
            auto& slots = activeSlots();
            const int at = juce::jlimit (0, (int) slots.size(), chain.getSelectedRow() + 1);
            FxSlot s;
            s.type = t;
            slots.insert (slots.begin() + at, s);
            rack.rebuild();
            refreshChain();
            chain.setSelectedRow (at);
            panel.setFx (&activeSlots()[(size_t) at]);
        };

        chain.onMoveUp = [this]
        {
            auto& slots = activeSlots();
            const int r = chain.getSelectedRow();
            if (r > 0 && r < (int) slots.size())
            {
                std::swap (slots[(size_t) r], slots[(size_t) (r - 1)]);
                rack.rebuild();
                refreshChain();
                chain.setSelectedRow (r - 1);
                panel.setFx (&activeSlots()[(size_t) (r - 1)]);
            }
        };

        chain.onMoveDown = [this]
        {
            auto& slots = activeSlots();
            const int r = chain.getSelectedRow();
            if (r >= 0 && r + 1 < (int) slots.size())
            {
                std::swap (slots[(size_t) r], slots[(size_t) (r + 1)]);
                rack.rebuild();
                refreshChain();
                chain.setSelectedRow (r + 1);
                panel.setFx (&activeSlots()[(size_t) (r + 1)]);
            }
        };

        chain.onDelete = [this]
        {
            auto& slots = activeSlots();
            const int r = chain.getSelectedRow();
            if (r >= 0 && r < (int) slots.size())
            {
                slots.erase (slots.begin() + r);
                rack.rebuild();
                refreshChain();
                panel.setFx (nullptr);
            }
        };

        panel.onChanged = [this]
        {
            auto& slots = activeSlots();
            const int r = chain.getSelectedRow();
            if (r >= 0 && r < (int) slots.size())
            {
                panel.commit (slots[(size_t) r]);
                rack.rebuild();
            }
        };

        refreshAll();
        refreshLayerControls();
        startTimerHz (24);
    }

    ~MainComponent() override { rack.stop(); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (AppleColors::Bg);

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
        g.drawText (L"音频设备", devArea.getX() + 14, devArea.getY() + 8, 200, 18,
                    juce::Justification::centredLeft, false);
        g.drawText (editingLayer ? L"✨ 空灵层（并行）" : L"🎤 主效果链",
                    chainArea.getX() + 14, chainArea.getY() + 8, 260, 18,
                    juce::Justification::centredLeft, false);
        g.drawText (L"参数", paramArea.getX() + 14, paramArea.getY() + 8, 200, 18,
                    juce::Justification::centredLeft, false);
        g.drawText (L"电平 / 状态", statArea.getX() + 14, statArea.getY() + 8, 200, 18,
                    juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        title.setBounds (r.removeFromTop (48));
        r.reduce (12, 8);

        // ---------------- ① 设备卡片 ----------------
        auto dev = r.removeFromTop (132);
        devArea = dev;
        auto d = dev.reduced (12, 26);
        const int rowH = 24, gap = 6, labelW = 58;

        auto row1 = d.removeFromTop (rowH); d.removeFromTop (gap);
        auto row2 = d.removeFromTop (rowH); d.removeFromTop (gap);
        auto row3 = d.removeFromTop (rowH); d.removeFromTop (gap);
        auto row4 = d.removeFromTop (rowH);

        lblHost.setBounds (row1.removeFromLeft (labelW));
        cbHost.setBounds (row1.removeFromLeft (juce::jmax (90, row1.getWidth() / 3 - 30)));
        row1.removeFromLeft (10);
        lblInDev.setBounds (row1.removeFromLeft (labelW));
        cbIn.setBounds (row1.removeFromLeft (juce::jmax (100, row1.getWidth() / 2 - 40)));
        row1.removeFromLeft (10);
        lblOutDev.setBounds (row1.removeFromLeft (labelW));
        cbOut.setBounds (row1);

        lblRate.setBounds (row2.removeFromLeft (labelW));
        cbRate.setBounds (row2.removeFromLeft (84));
        row2.removeFromLeft (8);
        lblBlock.setBounds (row2.removeFromLeft (labelW));
        cbBlock.setBounds (row2.removeFromLeft (84));
        row2.removeFromLeft (8);
        lblPreset.setBounds (row2.removeFromLeft (labelW));
        cbPreset.setBounds (row2.removeFromLeft (juce::jmax (90, row2.getWidth() - 160)));
        row2.removeFromLeft (8);
        btnSave.setBounds (row2.removeFromLeft (76)); row2.removeFromLeft (6);
        btnLoad.setBounds (row2.removeFromLeft (76));

        btnStart.setBounds   (row3.removeFromLeft (84));  row3.removeFromLeft (6);
        btnRefresh.setBounds (row3.removeFromLeft (72));  row3.removeFromLeft (6);
        btnBypass.setBounds  (row3.removeFromLeft (84));  row3.removeFromLeft (6);
        btnFxOn.setBounds    (row3.removeFromLeft (96));  row3.removeFromLeft (12);
        tgAuto.setBounds     (row3.removeFromLeft (110));

        lblGainIn.setBounds (row4.removeFromLeft (labelW));
        slGainIn.setBounds (row4.removeFromLeft (juce::jmax (100, row4.getWidth() / 3 - 50)));
        row4.removeFromLeft (10);
        lblGainOut.setBounds (row4.removeFromLeft (labelW));
        slGainOut.setBounds (row4.removeFromLeft (juce::jmax (100, row4.getWidth() / 2 - 50)));
        row4.removeFromLeft (10);
        lblPan.setBounds (row4.removeFromLeft (labelW));
        slPan.setBounds (row4);

        // ---------------- ② 效果链 + 参数 ----------------
        r.removeFromTop (8);
        auto mid = r.removeFromTop (juce::jmax (200, r.getHeight() - 104));
        auto left = mid.removeFromLeft (int (mid.getWidth() * 0.46));
        chainArea = left;

        auto l = left.reduced (12, 28);
        auto tabRow = l.removeFromTop (26);
        btnTabMain.setBounds (tabRow.removeFromLeft (104)); tabRow.removeFromLeft (6);
        btnTabLayer.setBounds (tabRow.removeFromLeft (104));
        l.removeFromTop (6);
        chain.setBounds (l);

        mid.removeFromLeft (8);
        paramArea = mid;
        auto pr = mid.reduced (12, 28);
        auto layerRow = pr.removeFromTop (24);
        tgLayer.setBounds (layerRow.removeFromLeft (96));
        layerRow.removeFromLeft (8);
        lblLayerGain.setBounds (layerRow.removeFromLeft (58));
        slLayerGain.setBounds (layerRow.removeFromLeft (juce::jmax (80, layerRow.getWidth() - 8)));
        pr.removeFromTop (6);
        panel.setBounds (pr);

        // ---------------- ③ 电平 ----------------
        r.removeFromTop (8);
        statArea = r.removeFromTop (96);
        auto s = statArea.reduced (12, 28);
        auto m1 = s.removeFromTop (24);
        lblIn.setBounds (m1.removeFromLeft (40));
        barIn.setBounds (m1.removeFromLeft (juce::jmax (120, m1.getWidth() / 2 - 60)));
        m1.removeFromLeft (14);
        lblOut.setBounds (m1.removeFromLeft (40));
        barOut.setBounds (m1.removeFromLeft (juce::jmax (120, m1.getWidth() - 90)));
        m1.removeFromLeft (8);
        lampClip.setBounds (m1);
        s.removeFromTop (6);
        lblStat.setBounds (s.removeFromTop (22));
    }

private:
    // ---------------- 小工具 ----------------
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

    void setupGainSlider (juce::Slider& s, double lo, double hi, double def, const juce::String& suffix)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
        s.setRange (lo, hi, 0.1);
        s.setTextValueSuffix (suffix);
        s.setValue (def, juce::dontSendNotification);
        s.setColour (juce::Slider::thumbColourId, AppleColors::Acc);
        addAndMakeVisible (s);
    }

    std::vector<FxSlot>& activeSlots()
    {
        return editingLayer ? rack.getLayerSlots() : rack.getSlots();
    }

    void setEditingLayer (bool layer)
    {
        editingLayer = layer;
        btnTabMain .setToggleState (! layer, juce::dontSendNotification);
        btnTabLayer.setToggleState (layer, juce::dontSendNotification);
        chain.setSelectedRow (0);
        refreshChain();
        repaint();
    }

    // ---------------- 刷新 ----------------
    void refreshAll()
    {
        cbHost.clear (juce::dontSendNotification);
        auto hosts = Rack::listHostTypes();
        for (int i = 0; i < hosts.size(); ++i)
            cbHost.addItem (hosts[i], i + 1);

        int asioId = 0;
        for (int i = 0; i < hosts.size(); ++i)
            if (hosts[i].containsIgnoreCase ("asio")) { asioId = i + 1; break; }

        cbHost.setSelectedId (asioId > 0 ? asioId : (hosts.isEmpty() ? 0 : 1),
                              juce::dontSendNotification);
        refreshDevices();
        refreshChain();
        refreshLayerControls();
    }

    void refreshDevices()
    {
        const auto host = cbHost.getText();

        cbIn.clear (juce::dontSendNotification);
        auto ins = Rack::listDevices (host, true);
        for (int i = 0; i < ins.size(); ++i)
            cbIn.addItem (ins[i], i + 1);
        if (! ins.isEmpty())
            cbIn.setSelectedId (1, juce::dontSendNotification);

        cbOut.clear (juce::dontSendNotification);
        auto outs = Rack::listDevices (host, false);
        for (int i = 0; i < outs.size(); ++i)
            cbOut.addItem (outs[i], i + 1);
        if (! outs.isEmpty())
            cbOut.setSelectedId (1, juce::dontSendNotification);
    }

    void refreshChain()
    {
        chain.setSlots (&activeSlots());
        const int r = chain.getSelectedRow();
        auto& slots = activeSlots();
        if (r >= 0 && r < (int) slots.size())
            panel.setFx (&slots[(size_t) r]);
        else
            panel.setFx (nullptr);
        repaint();
    }

    void refreshLayerControls()
    {
        tgLayer.setToggleState (rack.getLayerOn(), juce::dontSendNotification);
        slLayerGain.setValue ((double) rack.getLayerGainDb(), juce::dontSendNotification);
    }

    // ---------------- 动作 ----------------
    void toggleStart()
    {
        if (rack.isRunning())
        {
            rack.stop();
            btnStart.setButtonText (L"▶ 启动");
        }
        else
        {
            const double sr = cbRate.getText().getDoubleValue();
            const int    bs = cbBlock.getText().getIntValue();
            const auto err = rack.start (cbHost.getText(), cbIn.getText(), cbOut.getText(), sr, bs);
            if (err.isNotEmpty())
            {
                juce::NativeMessageBox::showMessageBoxAsync (
                    juce::MessageBoxIconType::WarningIcon,
                    "MioNo", juce::String (L"开流失败：\n") + err);
                btnStart.setButtonText (L"▶ 启动");
                return;
            }
            btnStart.setButtonText (L"■ 停止");
        }
        title.refresh();
    }

    void toggleMaximise()
    {
        auto* dw = dynamic_cast<juce::DocumentWindow*> (getTopLevelComponent());
        if (dw == nullptr) return;

        if (maximised)
        {
            dw->setBounds (restoreBounds);
            maximised = false;
        }
        else
        {
            restoreBounds = dw->getBounds();
            if (auto* disp = juce::Desktop::getInstance().getDisplays()
                                 .getDisplayForRect (dw->getScreenBounds()))
                dw->setBounds (disp->userArea);
            maximised = true;
        }
    }

    void saveConfig()
    {
        auto f = Rack::defaultConfigFile();
        const auto err = rack.saveConfig (f);
        status = err.isEmpty() ? (juce::String (L"已保存：") + f.getFullPathName()) : err;
    }

    void loadConfig()
    {
        auto f = Rack::defaultConfigFile();

        if (! f.existsAsFile())
        {
            chooser = std::make_unique<juce::FileChooser> (
                          L"选择配置文件", f.getParentDirectory(), "*.json");
            chooser->launchAsync (juce::FileBrowserComponent::openMode
                                    | juce::FileBrowserComponent::canSelectFiles,
                [this] (const juce::FileChooser& fc)
                {
                    auto picked = fc.getResult();
                    if (picked.getFullPathName().isEmpty()) return;
                    const auto err = rack.loadConfig (picked);
                    status = err.isEmpty() ? (juce::String (L"已载入：") + picked.getFileName()) : err;
                    afterConfigLoaded();
                });
            return;
        }

        const auto err = rack.loadConfig (f);
        status = err.isEmpty() ? (juce::String (L"已载入：") + f.getFileName()) : err;
        afterConfigLoaded();
    }

    void afterConfigLoaded()
    {
        cbPreset.setSelectedId (rack.getPresetIndex() + 1, juce::dontSendNotification);
        slGainIn .setValue ((double) rack.getInputGainDb(),  juce::dontSendNotification);
        slGainOut.setValue ((double) rack.getOutputGainDb(), juce::dontSendNotification);
        slPan    .setValue ((double) rack.getPan(),          juce::dontSendNotification);
        btnBypass.setButtonText (rack.getBypass() ? L"旁通·开" : L"旁通");
        refreshChain();
        refreshLayerControls();
    }

    void timerCallback() override
    {
        barIn .setLevels (rack.getInLevel(), rack.getInLevel() * 0.7f);
        barOut.setLevels (rack.getOutPeak(), rack.getOutRms());
        barIn .setClip (rack.getClipping());
        barOut.setClip (rack.getClipping());
        lampClip.setOn (rack.getClipping());

        // CLIP 灯亮 2 秒后自动清掉
        if (rack.getClipping())
        {
            if (++clipTicks > 48) { rack.clearClipping(); clipTicks = 0; }
        }
        else
        {
            clipTicks = 0;
        }

        juce::String s;
        if (status.isNotEmpty())
        {
            s = status;
            if (++statusTicks > 120) { status.clear(); statusTicks = 0; }   // 提示 5 秒后消失
        }
        else if (rack.isRunning())
            s = rack.deviceInfo()
              + juce::String (L"   单块 ") + juce::String (rack.getBlockMs(), 2) + L" ms"
              + juce::String (L"   丢帧 ") + juce::String (rack.getXruns());
        else
            s = L"未开流 · 选好输入/输出后点「▶ 启动」";

        if (lblStat.getText() != s)
            lblStat.setText (s, juce::dontSendNotification);

        title.refresh();
    }

    Rack rack;
    TitleBar title { rack };
    ChainList chain;
    FxParamPanel panel;

    juce::Label lblHost, lblInDev, lblOutDev, lblRate, lblBlock, lblPreset;
    juce::Label lblGainIn, lblGainOut, lblPan, lblLayerGain;
    juce::ComboBox cbHost, cbIn, cbOut, cbRate, cbBlock, cbPreset;
    juce::TextButton btnStart, btnRefresh, btnBypass, btnFxOn, btnSave, btnLoad;
    juce::TextButton btnTabMain, btnTabLayer;
    juce::ToggleButton tgAuto, tgLayer;
    juce::Slider slGainIn, slGainOut, slPan, slLayerGain;

    juce::Label lblIn, lblOut, lblStat;
    LevelBar barIn, barOut;
    Lamp lampClip { L"CLIP", AppleColors::Red };

    juce::Rectangle<int> devArea, chainArea, paramArea, statArea;

    bool editingLayer = false;
    bool maximised    = false;
    int  clipTicks    = 0;
    int  statusTicks  = 0;
    juce::Rectangle<int> restoreBounds;
    juce::String status;
    std::unique_ptr<juce::FileChooser> chooser;

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
        setTitleBarHeight (0);      // 把 JUCE 自己画的那条标题栏也去掉，只留自绘的
        setContentOwned (new MainComponent(), true);
        setResizable (true, false);
        setResizeLimits (820, 620, 10000, 10000);
        centreWithSize (1000, 700);
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
