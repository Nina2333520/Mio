// ============================================================================
//  MioNo - 主窗口（无边框苹果风）
//  v6.4：界面照 Python 版 MioNo.py 重排 —— ① 设备卡 / ② 预设卡 / ③ 标签页 / 状态栏
//        空灵层默认关闭；可以缩到状态栏（托盘）
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include "Rack.h"
#include "AppleLookAndFeel.h"
#include "AppleUI.h"
#include "TrayIcon.h"

namespace miono
{

// 增益 -> 「-12.3 dB」这种可读文字（Python 版 db_text 的对应物）
inline juce::String dbText (float gain)
{
    if (gain <= 0.00001f) return L"—";
    const float db = juce::Decibels::gainToDecibels (gain, -100.0f);
    if (db <= -99.0f) return L"—";
    return juce::String (db, 1) + L" dB";
}

class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    // 交给主窗口接管的两个动作 / 一个状态回报
    std::function<void()>  onHideToTray;      // 点「⇩ 缩到托盘」
    std::function<void (const juce::String&, bool)> onTrayUpdate;   // (提示文字, 是否在跑)

    bool trayOnClose = true;                  // 关窗口时缩到托盘（而不是退出）

    // 给主窗口 / 托盘菜单用（托盘里点「启动 / 停止」走这里）
    void trayToggleRun() { toggleStart(); }

    MainComponent()
    {
        setSize (1100, 860);

        addAndMakeVisible (title);
        addAndMakeVisible (tabs);

        // ---------------- ① 设备卡 ----------------
        setupLabel (lblIn,    L"输入");
        setupLabel (lblOut,   L"输出");
        setupLabel (lblHost,  L"驱动");
        setupLabel (lblRate,  L"采样率");
        setupLabel (lblBlock, L"块大小");
        setupLabel (lblGainIn,  L"输入增益");
        setupLabel (lblGainOut, L"输出增益");
        setupLabel (lblPan,     L"声像");

        for (auto* c : { &cbHost, &cbIn, &cbOut, &cbRate, &cbBlock })
        {
            c->setJustificationType (juce::Justification::centredLeft);
            addAndMakeVisible (c);
        }

        cbHost.onChange = [this] { refreshDevices(); };
        cbRate.addItem ("44100", 1);
        cbRate.addItem ("48000", 2);
        cbRate.addItem ("96000", 3);
        cbRate.setSelectedId (2, juce::dontSendNotification);
        cbBlock.addItem (L"0 = 自动（ASIO 推荐）", 1);
        cbBlock.addItem ("128",  2);
        cbBlock.addItem ("256",  3);
        cbBlock.addItem ("512",  4);
        cbBlock.addItem ("1024", 5);
        cbBlock.setSelectedId (3, juce::dontSendNotification);

        setupButton (btnRefresh, L"🔄 刷新", [this] { refreshAll(); });
        setupButton (btnStart,   L"▶ 启动", [this] { toggleStart(); });

        tgBypass.setButtonText (L"旁通全部效果（对比原声）");
        tgBypass.onClick = [this]
        {
            rack.setBypass (tgBypass.getToggleState());
            settingsChanged();
        };
        addAndMakeVisible (tgBypass);

        tgAuto.setButtonText (L"开机自启");
        tgAuto.setToggleState (Rack::getAutoStart(), juce::dontSendNotification);
        tgAuto.onClick = [this]
        {
            Rack::setAutoStart (tgAuto.getToggleState());
            status = tgAuto.getToggleState()
                        ? juce::String (L"已开启开机自启（开机后自动缩到状态栏）")
                        : juce::String (L"已关闭开机自启");
        };
        addAndMakeVisible (tgAuto);

        tgTray.setButtonText (L"关闭缩到托盘");
        trayOnClose = rack.getTrayOnClose();
        tgTray.setToggleState (trayOnClose, juce::dontSendNotification);
        tgTray.onClick = [this]
        {
            trayOnClose = tgTray.getToggleState();
            rack.setTrayOnClose (trayOnClose);
            status = trayOnClose ? juce::String (L"关窗口 = 缩到状态栏（右键图标可退出）")
                                 : juce::String (L"关窗口 = 直接退出程序");
            settingsChanged();
        };
        addAndMakeVisible (tgTray);

        setupButton (btnTray, L"⇩ 缩到托盘", [this] { if (onHideToTray != nullptr) onHideToTray(); });

        setupGainSlider (slGainIn,  -24.0, 24.0, 0.0, " dB");
        setupGainSlider (slGainOut, -24.0, 24.0, 0.0, " dB");
        setupGainSlider (slPan,      -1.0,  1.0, 0.0, "");

        slGainIn .onValueChange = [this]
        {
            rack.setInputGainDb ((float) slGainIn.getValue());
            settingsChanged();
        };
        slGainOut.onValueChange = [this]
        {
            rack.setOutputGainDb ((float) slGainOut.getValue());
            settingsChanged();
        };
        slPan    .onValueChange = [this]
        {
            rack.setPan ((float) slPan.getValue());
            updatePanLabel();
            settingsChanged();
        };

        lblPanVal.setText (L"居中", juce::dontSendNotification);
        lblPanVal.setFont (AppleLookAndFeel::getAppFont (12.5f));
        lblPanVal.setColour (juce::Label::textColourId, AppleColors::Dim);
        lblPanVal.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (lblPanVal);

        // 电平两行：输入 / 输出，各带一条 Peak+RMS 和 CLIP 灯 + 数字
        for (auto* l : { &lblMeterIn, &lblMeterOut })
        {
            l->setFont (AppleLookAndFeel::getAppFont (12.0f));
            l->setColour (juce::Label::textColourId, AppleColors::Dim);
            l->setJustificationType (juce::Justification::centredLeft);
            addAndMakeVisible (l);
        }
        lblMeterIn .setText (L"输入", juce::dontSendNotification);
        lblMeterOut.setText (L"输出", juce::dontSendNotification);

        for (auto* t : { &txtIn, &txtOut })
        {
            t->setFont (AppleLookAndFeel::getAppFont (12.0f));
            t->setColour (juce::Label::textColourId, AppleColors::Dim);
            t->setJustificationType (juce::Justification::centredLeft);
            t->setText (L"—", juce::dontSendNotification);
            addAndMakeVisible (t);
        }

        addAndMakeVisible (barIn);
        addAndMakeVisible (barOut);
        addAndMakeVisible (lampInClip);
        addAndMakeVisible (lampOutClip);

        // ---------------- ② 预设 / 配置 ----------------
        const auto ps = presets();
        for (int i = 0; i < (int) ps.size(); ++i)
        {
            auto* b = presetButtons.add (new juce::TextButton());
            b->setButtonText (juce::String (presetIcon (i)) + L" " + ps[(size_t) i].name);
            b->setClickingTogglesState (true);
            b->setRadioGroupId (2001, juce::dontSendNotification);
            b->onClick = [this, i] { applyPreset (i); };
            addAndMakeVisible (b);
        }
        auto* customButton = presetButtons.add (new juce::TextButton());
        customButton->setButtonText (juce::String (presetIcon (Rack::customPresetIndex())) + L" 自定义");
        customButton->setClickingTogglesState (true);
        customButton->setRadioGroupId (2001, juce::dontSendNotification);
        customButton->onClick = [this] { applyPreset (Rack::customPresetIndex()); };
        addAndMakeVisible (customButton);

        setupButton (btnSave, L"💾 保存配置", [this] { saveConfig(); });
        setupButton (btnLoad, L"📂 载入配置", [this] { loadConfig(); });

        // ---------------- ③ 标签页（主效果链 / 空灵层）----------------
        pageMain.onChanged  = [this] { chainEdited(); };
        pageLayer.onChanged = [this] { chainEdited(); };

        tabs.setTabBarDepth (34);
        tabs.setOutline (1);
        tabs.setColour (juce::TabbedComponent::backgroundColourId, AppleColors::Bg);
        tabs.setColour (juce::TabbedComponent::outlineColourId,    AppleColors::Line);
        tabs.addTab (L"🎤 主效果链", AppleColors::Card, &pageMain,  false);
        tabs.addTab (L"✨ 空灵层",   AppleColors::Card, &pageLayer, false);

        // ---------------- 状态栏 ----------------
        lblStat.setFont (AppleLookAndFeel::getAppFont (12.0f));
        lblStat.setColour (juce::Label::textColourId, AppleColors::Dim);
        lblStat.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (lblStat);

        // ---------------- 标题栏按钮 ----------------
        title.onClose    = [this] { if (auto* w = findParentComponentOfClass<juce::DocumentWindow>()) w->closeButtonPressed(); };
        title.onMinimise = [this] { if (auto* w = findParentComponentOfClass<juce::DocumentWindow>()) w->setMinimised (true); };
        title.onMaximise = [this] { toggleMaximise(); };

        // 自动恢复上次本地配置；不用每次手动点“载入配置”。
        const auto localConfig = Rack::defaultConfigFile();
        bool restored = false;
        if (localConfig.existsAsFile())
        {
            const auto err = rack.loadConfig (localConfig);
            if (err.isEmpty()) restored = true;
            else status = err;
        }
        afterConfigLoaded();
        refreshAll();
        if (restored)
            status = rack.getPresetIndex() == Rack::customPresetIndex()
                        ? juce::String (L"已恢复本地自定义效果链")
                        : juce::String (L"已恢复上次本地配置");

        startTimerHz (24);
    }

    ~MainComponent() override
    {
        flushAutoSave();
        rack.stop();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (AppleColors::Bg);

        auto card = [&] (juce::Rectangle<int> r)
        {
            g.setColour (AppleColors::Card);
            g.fillRoundedRectangle (r.toFloat(), 14.0f);
            g.setColour (AppleColors::Line.withAlpha (0.85f));
            g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 14.0f, 1.0f);
        };

        card (devArea);
        card (presetArea);

        g.setColour (AppleColors::Dim);
        g.setFont (AppleLookAndFeel::getBoldFont (12.5f));
        g.drawText (L"① 设备", devArea.getX() + 14, devArea.getY() + 7, 200, 18,
                    juce::Justification::centredLeft, false);
        g.drawText (L"② 预设 / 配置", presetArea.getX() + 14, presetArea.getY() + 7, 200, 18,
                    juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        title.setBounds (r.removeFromTop (52));
        r.reduce (14, 12);

        // ① 设备卡
        auto dev = r.removeFromTop (250);
        devArea = dev;
        layoutDeviceCard (dev);
        r.removeFromTop (11);

        // ② 预设卡
        auto pre = r.removeFromTop (60);
        presetArea = pre;
        layoutPresetCard (pre);
        r.removeFromTop (11);

        // 底部状态栏
        lblStat.setBounds (r.removeFromBottom (22).withTrimmedLeft (4));
        r.removeFromBottom (8);

        // ③ 剩下的全给标签页
        tabs.setBounds (r);
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
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 62, 20);
        s.setRange (lo, hi, 0.1);
        s.setTextValueSuffix (suffix);
        s.setValue (def, juce::dontSendNotification);
        s.setColour (juce::Slider::thumbColourId, AppleColors::Acc);
        addAndMakeVisible (s);
    }

    // ---------------- 布局：① 设备卡 ----------------
    void layoutDeviceCard (juce::Rectangle<int> dev)
    {
        auto d = dev.reduced (14, 9);
        d.removeFromTop (17);            // 卡片标题占位
        const int rowH = 24, gap = 6;

        auto r1 = d.removeFromTop (rowH); d.removeFromTop (gap);
        auto r2 = d.removeFromTop (rowH); d.removeFromTop (gap);
        auto r3 = d.removeFromTop (rowH); d.removeFromTop (gap);
        auto r4 = d.removeFromTop (rowH); d.removeFromTop (gap);
        auto r5 = d.removeFromTop (rowH); d.removeFromTop (gap + 3);
        auto r6 = d.removeFromTop (22);   d.removeFromTop (gap);
        auto r7 = d.removeFromTop (22);

        // 输入 + 🔄 刷新
        lblIn.setBounds (r1.removeFromLeft (40));
        btnRefresh.setBounds (r1.removeFromRight (84));
        r1.removeFromRight (6);
        cbIn.setBounds (r1);

        // 输出
        lblOut.setBounds (r2.removeFromLeft (40));
        cbOut.setBounds (r2);

        // 驱动 / 采样率 / 块大小
        lblHost.setBounds (r3.removeFromLeft (40));
        cbHost.setBounds (r3.removeFromLeft (juce::jmax (90, juce::jmin (200, r3.getWidth() / 4))));
        r3.removeFromLeft (12);
        lblRate.setBounds (r3.removeFromLeft (52));
        cbRate.setBounds (r3.removeFromLeft (92));
        r3.removeFromLeft (12);
        lblBlock.setBounds (r3.removeFromLeft (52));
        cbBlock.setBounds (r3.removeFromLeft (170));

        // 输入增益 / 输出增益 / 声像
        lblGainIn.setBounds (r4.removeFromLeft (60));
        slGainIn.setBounds (r4.removeFromLeft (juce::jmax (120, r4.getWidth() / 3 - 40)));
        r4.removeFromLeft (12);
        lblGainOut.setBounds (r4.removeFromLeft (60));
        slGainOut.setBounds (r4.removeFromLeft (juce::jmax (120, r4.getWidth() / 2 - 40)));
        r4.removeFromLeft (12);
        lblPan.setBounds (r4.removeFromLeft (40));
        lblPanVal.setBounds (r4.removeFromRight (58));
        slPan.setBounds (r4);

        // 勾选框 + 按钮
        tgBypass.setBounds (r5.removeFromLeft (188));
        r5.removeFromLeft (10);
        tgAuto.setBounds (r5.removeFromLeft (88));
        r5.removeFromLeft (10);
        tgTray.setBounds (r5.removeFromLeft (116));
        btnStart.setBounds (r5.removeFromRight (124));
        r5.removeFromRight (8);
        btnTray.setBounds (r5.removeFromRight (104));

        // 两行电平
        auto meterRow = [] (juce::Rectangle<int> row, juce::Label& lb, LevelBar& bar,
                            Lamp& lamp, juce::Label& txt)
        {
            lb.setBounds (row.removeFromLeft (34));
            lamp.setBounds (row.removeFromRight (54));
            row.removeFromRight (6);
            txt.setBounds (row.removeFromRight (190));
            row.removeFromRight (6);
            bar.setBounds (row);
        };
        meterRow (r6, lblMeterIn,  barIn,  lampInClip,  txtIn);
        meterRow (r7, lblMeterOut, barOut, lampOutClip, txtOut);
    }

    // ---------------- 布局：② 预设卡 ----------------
    void layoutPresetCard (juce::Rectangle<int> pre)
    {
        auto p = pre.reduced (14, 7);
        p.removeFromTop (16);

        auto row = p.removeFromTop (26);

        const int rightW = 110 + 7 + 110;      // 💾 保存 + 间距 + 📂 载入
        auto left = row.removeFromLeft (juce::jmax (240, row.getWidth() - rightW - 12));
        const int bw = juce::jlimit (100, 190, (left.getWidth() - 7 * (presetButtons.size() - 1))
                                                   / juce::jmax (1, presetButtons.size()));
        for (auto* b : presetButtons)
        {
            b->setBounds (left.removeFromLeft (bw));
            left.removeFromLeft (7);
        }

        row.removeFromLeft (12);
        btnSave.setBounds (row.removeFromLeft (110));
        row.removeFromLeft (7);
        btnLoad.setBounds (row.removeFromLeft (110));
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

    // ---------------- 预设 ----------------
    void updatePanLabel()
    {
        const int v = (int) std::round (slPan.getValue() * 100.0);
        const int av = (v < 0 ? -v : v);
        lblPanVal.setText (av < 3 ? juce::String (L"居中")
                                  : (v < 0 ? juce::String (L"左 ") + juce::String (-v) + L"%"
                                           : juce::String (L"右 ") + juce::String (v) + L"%"),
                           juce::dontSendNotification);
    }

    void syncGlobalControls()
    {
        slGainIn .setValue ((double) rack.getInputGainDb(),  juce::dontSendNotification);
        slGainOut.setValue ((double) rack.getOutputGainDb(), juce::dontSendNotification);
        slPan    .setValue ((double) rack.getPan(),          juce::dontSendNotification);
        tgBypass .setToggleState (rack.getBypass(), juce::dontSendNotification);
        updatePanLabel();
    }

    void applyPreset (int idx)
    {
        rack.applyPreset (idx);
        syncGlobalControls();
        updatePresetButtons();
        pageMain.refresh();
        pageLayer.refresh();

        const auto ps = presets();
        const juce::String name = idx == Rack::customPresetIndex()
                                    ? juce::String (L"自定义")
                                    : (idx >= 0 && idx < (int) ps.size()
                                           ? juce::String (ps[(size_t) idx].name)
                                           : juce::String());
        status = juce::String (L"已切换：") + juce::String (presetIcon (idx)) + L" " + name;
        queueAutoSave();
    }

    void updatePresetButtons()
    {
        const int active = rack.getPresetIndex();
        for (int i = 0; i < presetButtons.size(); ++i)
            presetButtons[i]->setToggleState (i == active, juce::dontSendNotification);
    }

    void chainEdited()
    {
        const bool wasCustom = rack.getPresetIndex() == Rack::customPresetIndex();
        rack.captureCustomPreset();
        updatePresetButtons();
        if (! wasCustom)
            status = L"效果链已切换到自定义，并会自动保存在本机";
        queueAutoSave();
        title.refresh();
    }

    void settingsChanged()
    {
        if (rack.getPresetIndex() == Rack::customPresetIndex())
            rack.captureCustomPreset();
        queueAutoSave();
    }

    void queueAutoSave()
    {
        autoSavePending = true;
        autoSaveTicks = 0;
    }

    void flushAutoSave()
    {
        if (! autoSavePending) return;
        autoSavePending = false;
        autoSaveTicks = 0;
        const auto f = Rack::defaultConfigFile();
        const auto err = rack.saveConfig (f);
        if (err.isNotEmpty())
            status = juce::String (L"本地自动保存失败：") + err;
    }

    // ---------------- 动作 ----------------
    void toggleStart()
    {
        if (rack.isRunning())
        {
            rack.stop();
            btnStart.setButtonText (L"▶ 启动");
            status = L"已停止";
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
                status = err;
                return;
            }
            btnStart.setButtonText (L"■ 停止");
            status = L"已启动";
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
        autoSavePending = false;
        autoSaveTicks = 0;
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
                    if (err.isEmpty()) queueAutoSave();
                });
            return;
        }

        const auto err = rack.loadConfig (f);
        status = err.isEmpty() ? (juce::String (L"已载入：") + f.getFileName()) : err;
        afterConfigLoaded();
        if (err.isEmpty()) queueAutoSave();
    }

    void afterConfigLoaded()
    {
        syncGlobalControls();
        trayOnClose = rack.getTrayOnClose();
        tgTray.setToggleState (trayOnClose, juce::dontSendNotification);

        updatePresetButtons();

        pageMain.refresh();
        pageLayer.refresh();
    }

    void timerCallback() override
    {
        if (autoSavePending && ++autoSaveTicks >= 12)  // 约 0.5 秒防抖
            flushAutoSave();

        const bool running = rack.isRunning();

        barIn .setLevels (rack.getInLevel(),  rack.getInRms());
        barOut.setLevels (rack.getOutPeak(),  rack.getOutRms());
        barIn .setClip (rack.getInClipping());
        barOut.setClip (rack.getOutClipping());
        lampInClip .setOn (rack.getInClipping());
        lampOutClip.setOn (rack.getOutClipping());

        txtIn .setText (juce::String (L"峰 ") + dbText (rack.getInLevel())
                        + juce::String (L" / 均 ") + dbText (rack.getInRms()),
                        juce::dontSendNotification);
        txtOut.setText (juce::String (L"峰 ") + dbText (rack.getOutPeak())
                        + juce::String (L" / 均 ") + dbText (rack.getOutRms()),
                        juce::dontSendNotification);

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
        else if (running)
            s = rack.deviceInfo()
              + juce::String (L"　占用 ") + juce::String ((double) rack.getLoadPercent(), 1) + L"%"
              + juce::String (L"　单块 ") + juce::String (rack.getBlockMs(), 2) + L" ms"
              + juce::String (L"　丢帧 ") + juce::String (rack.getXruns());
        else
            s = L"未开流 · 选好输入/输出后点「▶ 启动」";

        if (lblStat.getText() != s)
            lblStat.setText (s, juce::dontSendNotification);

        btnStart.setButtonText (running ? L"■ 停止" : L"▶ 启动");

        // 状态栏（托盘）图标的悬停提示跟着状态走
        if (onTrayUpdate != nullptr)
        {
            const juce::String tip = running
                ? (juce::String (L"MioNo · 运行中  ") + juce::String ((double) rack.getLoadPercent(), 0) + L"%")
                : juce::String (L"MioNo · 未开流");
            onTrayUpdate (tip, running);
        }

        title.refresh();
    }

    Rack rack;
    TitleBar title { rack };

    // ① 设备
    juce::Label lblIn, lblOut, lblHost, lblRate, lblBlock;
    juce::Label lblGainIn, lblGainOut, lblPan, lblPanVal;
    juce::Label lblMeterIn, lblMeterOut, txtIn, txtOut;
    juce::ComboBox cbHost, cbIn, cbOut, cbRate, cbBlock;
    juce::TextButton btnRefresh, btnStart, btnTray;
    juce::ToggleButton tgBypass, tgAuto, tgTray;
    juce::Slider slGainIn, slGainOut, slPan;
    LevelBar barIn, barOut;
    Lamp lampInClip  { L"CLIP", AppleColors::Red };
    Lamp lampOutClip { L"CLIP", AppleColors::Red };

    // ② 预设 / 配置
    juce::OwnedArray<juce::TextButton> presetButtons;
    juce::TextButton btnSave, btnLoad;

    // ③ 标签页（声明顺序很重要：tabs 要在两个页面之后销毁）
    ChainPage pageMain  { rack, false };
    ChainPage pageLayer { rack, true };
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };

    juce::Label lblStat;

    juce::Rectangle<int> devArea, presetArea;

    bool maximised   = false;
    int  clipTicks   = 0;
    int  statusTicks = 0;
    int  autoSaveTicks = 0;
    bool autoSavePending = false;
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
    MainWindow (juce::String name, bool startInTray = false)
        : juce::DocumentWindow (name, AppleColors::Bg, juce::DocumentWindow::allButtons)
    {
        // 关闭系统标题栏，标题栏由 TitleBar 组件自绘
        setUsingNativeTitleBar (false);
        setTitleBarButtonsRequired (0, false);
        setTitleBarHeight (0);      // 把 JUCE 自己画的那条标题栏也去掉，只留自绘的

        main = new MainComponent();
        setContentOwned (main, true);
        setResizable (true, false);
        setResizeLimits (920, 700, 10000, 10000);
        centreWithSize (1100, 860);

        // ---- 状态栏（托盘）图标 ----
        // 建出来就在状态栏上了；右键出菜单，左键把窗口叫回来。
        tray = std::make_unique<TrayIcon>();
        tray->onShowWindow = [this] { showFromTray(); };
        tray->onToggleRun  = [this] { if (main != nullptr) main->trayToggleRun(); };
        tray->onQuit       = [this] { quitNow(); };

        main->onHideToTray = [this] { hideToTray(); };
        main->onTrayUpdate = [this] (const juce::String& tip, bool run)
        {
            if (tray != nullptr) { tray->setRunning (run); tray->setTip (tip); }
        };

        if (startInTray)
        {
            // 开机自启：窗口根本不显示（也就不会闪一下），图标直接躺在状态栏上
            balloonShown = true;
            setVisible (false);
        }
        else
        {
            setVisible (true);
        }
    }

    // ---- 关窗口：缩到状态栏，还是退出？ ----
    void closeButtonPressed() override
    {
        if (main != nullptr && main->trayOnClose)
            hideToTray();
        else
            quitNow();
    }

    /** 缩到状态栏（窗口藏起来，音频继续跑，图标还在状态栏上）。 */
    void hideToTray()
    {
        setMinimised (false);
        setVisible (false);

        if (tray != nullptr && ! balloonShown)
        {
            balloonShown = true;      // 只提示一次，别老烦人
            tray->showInfoBubble (L"MioNo",
                                  L"已缩到状态栏，右键图标可以启动/停止或退出");
        }
    }

    /** 从状态栏把窗口叫回来。 */
    void showFromTray()
    {
        setVisible (true);
        setMinimised (false);
        toFront (true);
    }

    /** 真退出：先把托盘图标摘掉，免得系统里留个点不动的幽灵图标。 */
    void quitNow()
    {
        if (tray != nullptr)
            tray.reset();

        if (auto* app = juce::JUCEApplication::getInstance())
            app->systemRequestedQuit();
    }

    bool isInTray() const { return ! isVisible(); }

private:
    MainComponent* main = nullptr;
    std::unique_ptr<TrayIcon> tray;
    bool balloonShown = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};

} // namespace miono
