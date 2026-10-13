// ============================================================================
//  MioNo - 预设 / 机架核心
//  v7.0：整链（含空灵层）之后加总限幅兜底，避免叠加过冲直接硬削波
// ============================================================================
#pragma once

#include "FxEngine.h"

namespace miono
{

// ---------------------------------------------------------------------------
// 预设 —— 与 Python 版 EFFECT_FACTORY + PRESETS 对齐
// ---------------------------------------------------------------------------
struct PresetDef
{
    const wchar_t*      name;
    std::vector<FxSlot> chain;        // 主效果链
    bool                layerOn;      // 空灵层是否默认开
    float               layerGainDb;  // 空灵层音量
    std::vector<FxSlot> layer;        // 空灵层效果
};

inline FxSlot mkSlot (FxType t,
                      std::map<juce::String, float> ps = {},
                      bool on = true)
{
    FxSlot s;
    s.type = t;
    s.on = on;
    s.params = std::move (ps);
    return s;
}

inline std::vector<PresetDef> presets()
{
    return {
        { L"聊天 / 游戏",
          {
              mkSlot (FxType::SpectralDenoise, {{"strength", 0.55f}}),
              mkSlot (FxType::Highpass,        {{"freq", 90.0f}, {"q", 0.707f}}),
              mkSlot (FxType::NoiseGate,       {{"threshold", -48.0f}, {"attack", 3.0f}, {"release", 140.0f}}),
              mkSlot (FxType::PeakEQ,          {{"freq", 240.0f}, {"q", 1.0f}, {"gain", -3.0f}}),
              mkSlot (FxType::Compressor,      {{"threshold", -20.0f}, {"ratio", 2.5f}, {"attack", 8.0f}, {"release", 120.0f}, {"knee", 6.0f}}),
              mkSlot (FxType::Limiter,         {{"ceiling", -1.0f}, {"release", 60.0f}}),
          },
          true, -8.0f, {} },

        { L"唱歌",
          {
              mkSlot (FxType::SpectralDenoise, {{"strength", 0.45f}}),
              mkSlot (FxType::Highpass,        {{"freq", 100.0f}, {"q", 0.707f}}),
              mkSlot (FxType::PeakEQ,          {{"freq", 300.0f}, {"q", 1.2f}, {"gain", -2.5f}}),
              mkSlot (FxType::PeakEQ,          {{"freq", 3000.0f}, {"q", 1.0f}, {"gain", 1.5f}}),
              mkSlot (FxType::Compressor,      {{"threshold", -18.0f}, {"ratio", 3.0f}, {"attack", 10.0f}, {"release", 100.0f}, {"knee", 6.0f}}),
              mkSlot (FxType::Reverb,          {{"size", 0.45f}, {"damp", 0.5f}, {"wet", -12.0f}}),
              mkSlot (FxType::Delay,           {{"time", 250.0f}, {"feedback", 0.25f}, {"wet", -18.0f}}),
              mkSlot (FxType::Limiter,         {{"ceiling", -1.0f}, {"release", 60.0f}}),
          },
          true, -8.0f, {} },

        { L"空灵",
          {
              mkSlot (FxType::SpectralDenoise, {{"strength", 0.45f}}),
              mkSlot (FxType::Highpass,        {{"freq", 120.0f}, {"q", 0.707f}}),
              mkSlot (FxType::Compressor,      {{"threshold", -20.0f}, {"ratio", 3.0f}, {"attack", 12.0f}, {"release", 120.0f}, {"knee", 6.0f}}),
              mkSlot (FxType::Reverb,          {{"size", 0.78f}, {"damp", 0.42f}, {"wet", -3.0f}}),
              mkSlot (FxType::Delay,           {{"time", 375.0f}, {"feedback", 0.42f}, {"wet", -10.0f}}),
              mkSlot (FxType::Octave,          {{"mix", -14.0f}}),
              mkSlot (FxType::Limiter,         {{"ceiling", -1.0f}, {"release", 80.0f}}),
          },
          // 空灵层默认开启；需要干声时可在「✨ 空灵层」标签页手动关闭。
          true, -8.0f,
          {
              mkSlot (FxType::Reverb,          {{"size", 0.88f}, {"damp", 0.30f}, {"wet", -2.0f}}),
              mkSlot (FxType::Delay,           {{"time", 480.0f}, {"feedback", 0.36f}, {"wet", -11.0f}}),
          } },

        { L"电台厚声",
          {
              mkSlot (FxType::SpectralDenoise, {{"strength", 0.5f}}),
              mkSlot (FxType::Highpass,        {{"freq", 85.0f}, {"q", 0.707f}}),
              mkSlot (FxType::Lowpass,         {{"freq", 11000.0f}, {"q", 0.707f}}),
              mkSlot (FxType::PeakEQ,          {{"freq", 180.0f}, {"q", 1.0f}, {"gain", 3.5f}}),
              mkSlot (FxType::PeakEQ,          {{"freq", 3000.0f}, {"q", 1.0f}, {"gain", 2.5f}}),
              mkSlot (FxType::Compressor,      {{"threshold", -22.0f}, {"ratio", 4.0f}, {"attack", 6.0f}, {"release", 90.0f}, {"knee", 4.0f}}),
              mkSlot (FxType::Limiter,         {{"ceiling", -0.8f}, {"release", 50.0f}}),
          },
          true, -8.0f, {} },
    };
}

// ---------------------------------------------------------------------------
// 机架
// ---------------------------------------------------------------------------
class Rack : public juce::AudioIODeviceCallback
{
public:
    Rack()  { applyPreset (1); }        // 默认唱歌
    ~Rack() override { stop(); }

    // ---------------- 驱动类型 / 设备枚举 ----------------
    // JUCE 8 里 AudioIODeviceType::getAvailableDeviceTypes() 已经没有了，
    // 要拿驱动类型列表得走 AudioDeviceManager。
    static const juce::OwnedArray<juce::AudioIODeviceType>& availableTypes()
    {
        static juce::AudioDeviceManager* mgr = new juce::AudioDeviceManager();
        return mgr->getAvailableDeviceTypes();
    }

    static juce::StringArray listHostTypes()
    {
        juce::StringArray a;
        for (auto* t : availableTypes())
            a.add (t->getTypeName());
        return a;
    }

    // wantInput = true 取输入设备列表，false 取输出设备列表
    static juce::StringArray listDevices (const juce::String& hostType, bool wantInput)
    {
        juce::StringArray out;
        for (auto* t : availableTypes())
            if (t->getTypeName() == hostType)
            {
                t->scanForDevices();
                out = t->getDeviceNames (wantInput);
                break;
            }
        return out;
    }

    static juce::StringArray listChannels (const juce::String& hostType,
                                           const juce::String& dev, bool input)
    {
        juce::StringArray out;
        for (auto* t : availableTypes())
            if (t->getTypeName() == hostType)
            {
                t->scanForDevices();
                if (std::unique_ptr<juce::AudioIODevice> d { t->createDevice (dev, dev) })
                    out = input ? d->getInputChannelNames() : d->getOutputChannelNames();
                break;
            }
        return out;
    }

    // ---------------- 开流 ----------------
    juce::String start (const juce::String& hostType,
                        const juce::String& inputDeviceName,
                        const juce::String& outputDeviceName,
                        double sampleRate,
                        int blockSize)
    {
        stop();
        juce::String err;

        juce::AudioIODeviceType* chosen = nullptr;
        for (auto* t : availableTypes())
            if (t->getTypeName() == hostType) { chosen = t; break; }

        if (chosen == nullptr && ! availableTypes().isEmpty())
            chosen = availableTypes()[0];

        if (chosen == nullptr)
            return L"找不到任何音频驱动";

        chosen->scanForDevices();

        // 注意参数顺序：JUCE 是 createDevice (输出名, 输入名)
        std::unique_ptr<juce::AudioIODevice> dev (
            chosen->createDevice (outputDeviceName, inputDeviceName));

        if (dev == nullptr)
            return L"打不开设备：" + outputDeviceName + L" / " + inputDeviceName;

        // 采样率 / 块大小对齐设备支持值
        auto srOk = dev->getAvailableSampleRates();
        if (srOk.size() > 0)
        {
            bool found = false;
            for (auto v : srOk)
                if (std::abs (v - sampleRate) < 1.0) { found = true; break; }
            if (! found)
                sampleRate = srOk[0];
        }

        // 块大小对齐设备支持值。
        // ⚠️ blockSize <= 0 表示「自动」——千万别在这里改成 bsOk[0]，
        //    那样就变成强制用最小块了。JUCE 里 0 是有含义的：
        //      ASIO  ：bufferSize < 8  → 用驱动的 preferred size
        //      WASAPI：bufferSize <= 0 → 用 defaultBufferSize
        auto bsOk = dev->getAvailableBufferSizes();
        if (bsOk.size() > 0 && blockSize > 0)
        {
            bool found = false;
            for (auto v : bsOk)
                if (v == blockSize) { found = true; break; }
            if (! found)
                blockSize = bsOk[0];
        }

        // 通道掩码：优先用设备给的默认通道，否则把所有通道都打开
        juce::BigInteger inMask, outMask;
        if (auto def = dev->getDefaultInputChannels())  inMask  = *def;
        else inMask.setRange (0, dev->getInputChannelNames().size(), true);

        if (auto def = dev->getDefaultOutputChannels()) outMask = *def;
        else outMask.setRange (0, dev->getOutputChannelNames().size(), true);

        err = dev->open (inMask, outMask, sampleRate, blockSize);
        if (err.isNotEmpty())
            return err;

        currentDevice     = std::move (dev);
        currentHost       = hostType;
        currentInputName  = inputDeviceName;
        currentOutputName = outputDeviceName;

        currentSampleRate = currentDevice->getCurrentSampleRate();
        currentBlockSize  = currentDevice->getCurrentBufferSizeSamples();
        rebuild();

        currentDevice->start (this);
        running = true;
        return {};
    }

    void stop()
    {
        if (currentDevice != nullptr && running)
            currentDevice->stop();
        running = false;
    }

    bool isRunning() const { return running; }

    double getSampleRate() const { return currentSampleRate; }
    int    getBlockSize()  const { return currentBlockSize; }
    juce::String getHost()       const { return currentHost; }
    juce::String getInputName()  const { return currentInputName; }
    juce::String getOutputName() const { return currentOutputName; }

    void setPreferredDeviceSettings (const juce::String& host,
                                     const juce::String& input,
                                     const juce::String& output,
                                     double sampleRate,
                                     int blockSize)
    {
        preferredHost = host;
        preferredInputName = input;
        preferredOutputName = output;
        preferredSampleRate = sampleRate;
        preferredBlockSize = blockSize;
    }
    juce::String getPreferredHost() const { return preferredHost; }
    juce::String getPreferredInputName() const { return preferredInputName; }
    juce::String getPreferredOutputName() const { return preferredOutputName; }
    double getPreferredSampleRate() const { return preferredSampleRate; }
    int getPreferredBlockSize() const { return preferredBlockSize; }

    juce::String deviceInfo() const
    {
        if (currentDevice == nullptr) return L"未连接";
        return currentHost + L" · " + currentInputName + L" → " + currentOutputName
             + juce::String (L" · ") + juce::String (currentSampleRate, 0) + L" Hz"
             + juce::String (L" · ") + juce::String (currentBlockSize) + L" smp";
    }

    // ---------------- 增益 / 声像 ----------------
    void  setInputGainDb  (float db) { inputGainDb .store (juce::jlimit (-24.0f, 24.0f, db)); }
    float getInputGainDb()   const   { return inputGainDb .load(); }
    void  setOutputGainDb (float db) { outputGainDb.store (juce::jlimit (-24.0f, 24.0f, db)); }
    float getOutputGainDb()  const   { return outputGainDb.load(); }
    void  setPan (float p)           { pan.store (juce::jlimit (-1.0f, 1.0f, p)); }
    float getPan()           const   { return pan.load(); }

    // ---------------- 效果链 / 自定义预设 ----------------
    static constexpr int customPresetIndex() { return 4; }
    int getPresetIndex() const { return presetIndex; }
    bool hasCustomPreset() const { return customPresetValid; }

    /** 当前编辑完成后快照为“自定义”。主输出增益/声像/总旁通是全局控制，不属于预设。 */
    void captureCustomPreset()
    {
        const juce::ScopedLock sl (chainLock);
        customSlots      = slots;
        customLayerSlots = layerSlots;
        customLayerOn.store (layerOn.load());
        customLayerGainDb.store (layerGainDb.load());
        customPresetValid = true;
        presetIndex = customPresetIndex();
    }

    void applyPreset (int idx)
    {
        auto ps = presets();
        if (idx == customPresetIndex())
        {
            {
                const juce::ScopedLock sl (chainLock);
                if (! customPresetValid)
                {
                    // 第一次点“自定义”：用当前声音作底稿，不从空链开始。
                    customSlots      = slots;
                    customLayerSlots = layerSlots;
                    customLayerOn.store (layerOn.load());
                    customLayerGainDb.store (layerGainDb.load());
                    customPresetValid = true;
                }
                else
                {
                    slots      = customSlots;
                    layerSlots = customLayerSlots;
                    layerOn.store (customLayerOn.load());
                    layerGainDb.store (customLayerGainDb.load());
                }
                presetIndex = customPresetIndex();
            }
            rebuild();
            return;
        }

        if (idx < 0 || idx >= (int) ps.size()) idx = 0;
        presetIndex = idx;
        {
            const juce::ScopedLock sl (chainLock);
            slots      = ps[(size_t) idx].chain;
            layerSlots = ps[(size_t) idx].layer;
        }
        layerOn.store (ps[(size_t) idx].layerOn);
        layerGainDb.store (ps[(size_t) idx].layerGainDb);
        rebuild();
    }

    std::vector<FxSlot>& getSlots()      { return slots; }
    std::vector<FxSlot>& getLayerSlots() { return layerSlots; }

    void  setLayerOn (bool b)        { layerOn.store (b); }
    bool  getLayerOn() const         { return layerOn.load(); }
    void  setLayerGainDb (float db)  { layerGainDb.store (juce::jlimit (-40.0f, 6.0f, db)); }
    float getLayerGainDb() const     { return layerGainDb.load(); }

    void setBypass (bool b) { bypassAll.store (b); }
    bool getBypass() const  { return bypassAll.load(); }

    // ---------------- 状态栏（托盘） ----------------
    void setTrayOnClose (bool b) { trayOnClose.store (b); }
    bool getTrayOnClose() const  { return trayOnClose.load(); }

    void setAutoStartAudio (bool b) { autoStartAudio.store (b); }
    bool getAutoStartAudio() const  { return autoStartAudio.load(); }

    void rebuild()
    {
        const juce::ScopedLock sl (chainLock);

        runtimes.clear();
        runtimes.reserve (slots.size());
        for (const auto& s : slots)
        {
            FxRuntime r;
            r.prepare (currentSampleRate);
            r.configure (s);
            runtimes.push_back (std::move (r));
        }

        layerRuntimes.clear();
        layerRuntimes.reserve (layerSlots.size());
        for (const auto& s : layerSlots)
        {
            FxRuntime r;
            r.prepare (currentSampleRate);
            r.configure (s);
            layerRuntimes.push_back (std::move (r));
        }

        // 总输出限幅：空灵层是在主链（含末端限幅器）之后叠加的，
        // 这里单独准备一级砖墙限幅，兜住最终电平，避免直接硬削波。
        outputLimiter.ceilingDb = -1.0f;
        outputLimiter.releaseMs = 60.0f;
        outputLimiter.sr        = currentSampleRate;
        outputLimiter.reset();
    }

    // ---------------- 电平 / 统计 ----------------
    float  getInLevel()   const { return inLevel.load(); }
    float  getInRms()     const { return inRms.load(); }
    float  getOutPeak()   const { return outPeak.load(); }
    float  getOutRms()    const { return outRms.load(); }
    bool   getClipping()  const { return clipping.load(); }
    bool   getInClipping() const { return inClip.load(); }
    bool   getOutClipping() const { return clipping.load(); }
    void   clearClipping()      { clipping.store (false); inClip.store (false); }
    double getBlockMs()   const { return blockMs.load(); }
    int    getXruns()     const { return xruns.load(); }

    // 处理负载 = 单块耗时 / 单块时间预算（跟 REAPER 那个 CPU 表一个意思）
    float getLoadPercent() const
    {
        const double budgetMs = 1000.0 * (double) currentBlockSize
                                       / juce::jmax (1.0, currentSampleRate);
        if (budgetMs <= 0.0) return 0.0f;
        return (float) (blockMs.load() / budgetMs * 100.0);
    }

    // ---------------- 配置存盘 ----------------
    static juce::File defaultConfigFile()
    {
        // 自动恢复文件放用户数据目录，避免程序装在只读目录时写入失败。
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile (juce::String (L"MioNo"))
                   .getChildFile (juce::String (L"MioNo配置.json"));
    }

    /** 迁移旧版放在 exe 同目录的自动保存文件。 */
    static juce::File legacyConfigFile()
    {
        return juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                   .getParentDirectory().getChildFile (juce::String (L"MioNo配置.json"));
    }

    juce::String saveConfig (const juce::File& f) const
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("version",   10);
        o->setProperty ("preset",    presetIndex);
        o->setProperty ("inGain",    (double) inputGainDb.load());
        o->setProperty ("outGain",   (double) outputGainDb.load());
        o->setProperty ("pan",       (double) pan.load());
        o->setProperty ("bypass",    bypassAll.load());
        o->setProperty ("layerOn",   layerOn.load());
        o->setProperty ("trayOnClose", trayOnClose.load());
        o->setProperty ("autoStartAudio", autoStartAudio.load());
        o->setProperty ("layerGain", (double) layerGainDb.load());
        o->setProperty ("host",      preferredHost.isNotEmpty() ? preferredHost : currentHost);
        o->setProperty ("inDev",     preferredInputName.isNotEmpty() ? preferredInputName : currentInputName);
        o->setProperty ("outDev",    preferredOutputName.isNotEmpty() ? preferredOutputName : currentOutputName);
        o->setProperty ("rate",      preferredSampleRate);
        o->setProperty ("block",     preferredBlockSize);

        {
            const juce::ScopedLock sl (chainLock);
            o->setProperty ("chain", slotsToVar (slots));
            o->setProperty ("layer", slotsToVar (layerSlots));
            o->setProperty ("customValid", customPresetValid);
            o->setProperty ("customChain", slotsToVar (customSlots));
            o->setProperty ("customLayer", slotsToVar (customLayerSlots));
        }
        o->setProperty ("customLayerOn",   customLayerOn.load());
        o->setProperty ("customLayerGain", (double) customLayerGainDb.load());

        if (! f.getParentDirectory().createDirectory())
            return L"无法创建配置目录：" + f.getParentDirectory().getFullPathName();

        const juce::var root (o);
        if (! f.replaceWithText (juce::JSON::toString (root, false)))
            return L"写入失败：" + f.getFullPathName();

        return {};
    }

    juce::String loadConfig (const juce::File& f)
    {
        if (! f.existsAsFile())
            return L"文件不存在：" + f.getFullPathName();

        const auto root = juce::JSON::parse (f);
        if (! root.isObject())
            return L"配置文件格式不对：必须是 JSON 对象";

        const auto chainData = root.getProperty ("chain", juce::var());
        const auto layerData = root.getProperty ("layer", juce::var());
        if (! chainData.isArray() || (! layerData.isVoid() && ! layerData.isArray()))
            return L"配置文件格式不对：缺少有效的主效果链/空灵层数据";

        // 先完整解析到临时状态，校验成功后再一次性应用，避免坏文件载入一半。
        auto newSlots = slotsFromVar (chainData);
        auto newLayerSlots = layerData.isArray() ? slotsFromVar (layerData)
                                                 : std::vector<FxSlot>();
        const int storedConfigVersion = (int) root.getProperty ("version", 0);
        const int storedPreset = (int) root.getProperty ("preset", 1);
        const int newPreset = (storedPreset >= 0 && storedPreset <= customPresetIndex())
                                ? storedPreset : 1;
        const auto customChainData = root.getProperty ("customChain", juce::var());
        const auto customLayerData = root.getProperty ("customLayer", juce::var());
        bool newCustomValid = (bool) root.getProperty ("customValid", false)
                           && customChainData.isArray() && customLayerData.isArray();
        auto newCustomSlots = newCustomValid ? slotsFromVar (customChainData)
                                             : std::vector<FxSlot>();
        auto newCustomLayerSlots = newCustomValid ? slotsFromVar (customLayerData)
                                                  : std::vector<FxSlot>();

        const float newInGain = (float) (double) root.getProperty ("inGain", 0.0);
        const float newOutGain = (float) (double) root.getProperty ("outGain", 0.0);
        const float newPan = (float) (double) root.getProperty ("pan", 0.0);
        const float newLayerGain = (float) (double) root.getProperty ("layerGain", -8.0);
        const float newCustomLayerGain = (float) (double) root.getProperty ("customLayerGain", -8.0);
        bool newLayerOn = (bool) root.getProperty ("layerOn", true);
        const bool newCustomLayerOn = (bool) root.getProperty ("customLayerOn", false);
        // v8 及更早版本的内置预设默认把空灵层关掉；升级后采用新默认开启。
        // 自定义预设保留用户明确保存的开关状态。
        if (storedConfigVersion < 9 && newPreset != customPresetIndex())
            newLayerOn = true;

        inputGainDb.store (juce::jlimit (-24.0f, 24.0f, newInGain));
        outputGainDb.store (juce::jlimit (-24.0f, 24.0f, newOutGain));
        pan.store (juce::jlimit (-1.0f, 1.0f, newPan));
        layerGainDb.store (juce::jlimit (-40.0f, 6.0f, newLayerGain));
        bypassAll.store ((bool) root.getProperty ("bypass", false));
        layerOn.store (newLayerOn);
        trayOnClose.store ((bool) root.getProperty ("trayOnClose", true));
        autoStartAudio.store ((bool) root.getProperty ("autoStartAudio", true));
        preferredHost = root.getProperty ("host", preferredHost.isNotEmpty() ? preferredHost : currentHost).toString();
        preferredInputName = root.getProperty ("inDev", preferredInputName.isNotEmpty() ? preferredInputName : currentInputName).toString();
        preferredOutputName = root.getProperty ("outDev", preferredOutputName.isNotEmpty() ? preferredOutputName : currentOutputName).toString();
        preferredSampleRate = (double) root.getProperty ("rate", preferredSampleRate > 0.0 ? preferredSampleRate : currentSampleRate);
        preferredBlockSize = juce::jmax (0, (int) root.getProperty ("block", preferredBlockSize));

        customLayerOn.store (newCustomLayerOn);
        customLayerGainDb.store (juce::jlimit (-40.0f, 6.0f, newCustomLayerGain));
        presetIndex = newPreset;
        {
            const juce::ScopedLock sl (chainLock);
            slots = std::move (newSlots);
            layerSlots = std::move (newLayerSlots);
            customPresetValid = newCustomValid;
            customSlots = std::move (newCustomSlots);
            customLayerSlots = std::move (newCustomLayerSlots);

            // 兼容旧配置：旧版若正在使用自定义链但没有独立快照，就用当前链补建。
            if (! customPresetValid && presetIndex == customPresetIndex())
            {
                customSlots = slots;
                customLayerSlots = layerSlots;
                customLayerOn.store (layerOn.load());
                customLayerGainDb.store (layerGainDb.load());
                customPresetValid = true;
            }
        }
        rebuild();
        return {};
    }

    // ---------------- 开机自启（写 HKCU\...\Run） ----------------
    static bool getAutoStart()
    {
       #if JUCE_WINDOWS
        return juce::WindowsRegistry::valueExists (autoStartKey());
       #else
        return false;
       #endif
    }

    static void setAutoStart (bool on)
    {
       #if JUCE_WINDOWS
        // 注意：开机自启带上 --tray，开机后【直接缩到状态栏】，
        // 不弹窗口（这正是"开机自启缩到状态栏"的实现方式）。
        if (on) juce::WindowsRegistry::setValue (autoStartKey(), "\"" + exePath() + "\" --tray");
        else    juce::WindowsRegistry::deleteValue (autoStartKey());
       #else
        juce::ignoreUnused (on);
       #endif
    }

    // ---------------- AudioIODeviceCallback ----------------
    void audioDeviceIOCallbackWithContext (const float* const* in, int numIn,
                                           float* const* out, int numOut,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext&) override
    {
        const auto t0 = juce::Time::getHighResolutionTicks();

        for (int ch = 0; ch < numOut; ++ch)
            if (out[ch] != nullptr)
                juce::FloatVectorOperations::clear (out[ch], numSamples);

        if (numIn <= 0 || numOut <= 0 || in == nullptr || in[0] == nullptr)
        {
            recordTime (t0);
            return;
        }

        const float* src = in[0];

        const juce::ScopedTryLock stl (chainLock);
        if (! stl.isLocked())
        {
            xruns.fetch_add (1);
            if (out[0] != nullptr)
                juce::FloatVectorOperations::copy (out[0], src, numSamples);
            recordTime (t0);
            return;
        }

        const float inGain   = juce::Decibels::decibelsToGain (inputGainDb.load());
        const float outGain  = juce::Decibels::decibelsToGain (outputGainDb.load());
        const bool  bypass   = bypassAll.load();
        const bool  useLayer = layerOn.load() && ! layerRuntimes.empty();
        const float layGain  = useLayer ? juce::Decibels::decibelsToGain (layerGainDb.load()) : 0.0f;

        // 等功率声像（-1 全左，0 居中，+1 全右）
        const float panAngle = (pan.load() + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
        const float panL = std::cos (panAngle);
        const float panR = std::sin (panAngle);

        const bool stereoOut = (numOut >= 2 && out[1] != nullptr);

        float peakIn = 0.0f, peakOut = 0.0f, sumSq = 0.0f, sumSqIn = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dry = src[i];
            const float x   = dry * inGain;
            peakIn = juce::jmax (peakIn, std::fabs (x));
            sumSqIn += x * x;

            float y = x;
            if (! bypass)
            {
                for (auto& r : runtimes)
                    y = r.process (y);

                if (useLayer)
                {
                    float l = x;
                    for (auto& r : layerRuntimes)
                        l = r.process (l);
                    y += l * layGain;
                }
            }

            y *= outGain;

            // 总限幅兜底：空灵层叠加在主链之后，主链末端的限幅器管不到它，
            // 这里对最终信号再过一级砖墙限幅（瞬时起音、平滑释放），
            // 把过冲压回 -1 dBFS 以内，而不是直接硬削波产生破音。
            y = outputLimiter.process (y);

            // 极端情况下的最后一道保护
            if (y >  1.0f) y =  1.0f;
            if (y < -1.0f) y = -1.0f;

            if (stereoOut)
            {
                out[0][i] = y * panL;
                out[1][i] = y * panR;
            }
            else if (out[0] != nullptr)
            {
                out[0][i] = y;
            }

            peakOut = juce::jmax (peakOut, std::fabs (y));
            sumSq += y * y;
        }

        inLevel.store (peakIn);
        inRms  .store (std::sqrt (sumSqIn / (float) numSamples));
        outPeak.store (peakOut);
        outRms .store (std::sqrt (sumSq / (float) numSamples));
        if (peakIn >= 0.999f)  inClip.store (true);
        if (peakOut >= 0.999f) clipping.store (true);

        recordTime (t0);
    }

    void audioDeviceAboutToStart (juce::AudioIODevice* d) override
    {
        currentSampleRate = d->getCurrentSampleRate();
        currentBlockSize  = d->getCurrentBufferSizeSamples();
        rebuild();
    }

    void audioDeviceStopped() override {}

private:
    static juce::String exePath()
    {
        return juce::File::getSpecialLocation (juce::File::currentExecutableFile).getFullPathName();
    }

    static juce::String autoStartKey()
    {
        return "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\\MioNo";
    }

    static juce::var slotsToVar (const std::vector<FxSlot>& v)
    {
        juce::Array<juce::var> arr;
        for (const auto& s : v)
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("type", (int) s.type);
            o->setProperty ("on",   s.on);

            auto* ps = new juce::DynamicObject();
            for (const auto& kv : s.params)
                ps->setProperty (juce::Identifier (kv.first), (double) kv.second);

            o->setProperty ("params", juce::var (ps));
            arr.add (juce::var (o));
        }
        return juce::var (arr);
    }

    static std::vector<FxSlot> slotsFromVar (const juce::var& v)
    {
        std::vector<FxSlot> out;
        if (! v.isArray())
            return out;

        for (int i = 0; i < v.size(); ++i)
        {
            const auto& e = v[i];
            if (! e.isObject())
                continue;

            FxSlot s;
            s.type = fxTypeFromIndex ((int) e.getProperty ("type", 0));
            s.on   = (bool) e.getProperty ("on", true);

            if (auto* obj = e.getProperty ("params", juce::var()).getDynamicObject())
                for (const auto& p : obj->getProperties())
                    s.params[p.name.toString()] = (float) (double) p.value;

            out.push_back (std::move (s));
        }
        return out;
    }

    void recordTime (juce::int64 t0)
    {
        const auto t1 = juce::Time::getHighResolutionTicks();
        blockMs.store (juce::Time::highResolutionTicksToSeconds (t1 - t0) * 1000.0);
    }

    std::unique_ptr<juce::AudioIODevice> currentDevice;
    juce::String currentHost, currentInputName, currentOutputName;
    double currentSampleRate = 48000.0;
    int    currentBlockSize  = 256;
    juce::String preferredHost, preferredInputName, preferredOutputName;
    double preferredSampleRate = 48000.0;
    int preferredBlockSize = 256;

    std::vector<FxSlot>    slots;
    std::vector<FxRuntime> runtimes;
    std::vector<FxSlot>    layerSlots;
    std::vector<FxRuntime> layerRuntimes;
    std::vector<FxSlot>    customSlots;
    std::vector<FxSlot>    customLayerSlots;
    LimiterFx              outputLimiter;   // 总输出限幅（兜住空灵层叠加后的过冲）
    juce::CriticalSection  chainLock;

    int  presetIndex = 1;
    bool running     = false;
    bool customPresetValid = false;

    std::atomic<float> customLayerGainDb { -8.0f };
    std::atomic<bool>  customLayerOn     { false };

    std::atomic<float> inRms       { 0.0f };
    std::atomic<bool>  inClip      { false };
    std::atomic<bool>  bypassAll   { false };
    std::atomic<bool>  trayOnClose  { true };   // 关窗口时缩到状态栏
    std::atomic<bool>  autoStartAudio { true }; // 打开软件后自动开流
    std::atomic<bool>  layerOn      { true };
    std::atomic<float> layerGainDb { -8.0f };
    std::atomic<float> inputGainDb { 0.0f };
    std::atomic<float> outputGainDb{ 0.0f };
    std::atomic<float> pan         { 0.0f };

    std::atomic<float>  inLevel  { 0.0f };
    std::atomic<float>  outPeak  { 0.0f };
    std::atomic<float>  outRms   { 0.0f };
    std::atomic<bool>   clipping { false };
    std::atomic<double> blockMs  { 0.0 };
    std::atomic<int>    xruns    { 0 };
};

} // namespace miono
