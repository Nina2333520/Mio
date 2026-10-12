// ============================================================================
//  MioNo - 预设 / 机架核心
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
    const char* name;
    std::vector<FxSlot> chain;
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
        { u8"聊天 / 游戏", {
            mkSlot (FxType::SpectralDenoise, {{"strength", 0.55f}}),
            mkSlot (FxType::Highpass,        {{"freq", 90.0f}, {"q", 0.707f}}),
            mkSlot (FxType::NoiseGate,       {{"threshold", -48.0f}, {"attack", 3.0f}, {"release", 140.0f}}),
            mkSlot (FxType::PeakEQ,          {{"freq", 240.0f}, {"q", 1.0f}, {"gain", -3.0f}}),
            mkSlot (FxType::Compressor,      {{"threshold", -20.0f}, {"ratio", 2.5f}, {"attack", 8.0f}, {"release", 120.0f}, {"knee", 6.0f}}),
            mkSlot (FxType::Limiter,         {{"ceiling", -1.0f}, {"release", 60.0f}}),
        }},
        { u8"唱歌", {
            mkSlot (FxType::SpectralDenoise, {{"strength", 0.45f}}),
            mkSlot (FxType::Highpass,        {{"freq", 100.0f}, {"q", 0.707f}}),
            mkSlot (FxType::PeakEQ,          {{"freq", 300.0f}, {"q", 1.2f}, {"gain", -2.5f}}),
            mkSlot (FxType::PeakEQ,          {{"freq", 3000.0f}, {"q", 1.0f}, {"gain", 1.5f}}),
            mkSlot (FxType::Compressor,      {{"threshold", -18.0f}, {"ratio", 3.0f}, {"attack", 10.0f}, {"release", 100.0f}, {"knee", 6.0f}}),
            mkSlot (FxType::Reverb,          {{"size", 0.45f}, {"damp", 0.5f}, {"wet", -12.0f}}),
            mkSlot (FxType::Delay,           {{"time", 250.0f}, {"feedback", 0.25f}, {"wet", -18.0f}}),
            mkSlot (FxType::Limiter,         {{"ceiling", -1.0f}, {"release", 60.0f}}),
        }},
        { u8"空灵", {
            mkSlot (FxType::SpectralDenoise, {{"strength", 0.45f}}),
            mkSlot (FxType::Highpass,        {{"freq", 120.0f}, {"q", 0.707f}}),
            mkSlot (FxType::Compressor,      {{"threshold", -20.0f}, {"ratio", 3.0f}, {"attack", 12.0f}, {"release", 120.0f}, {"knee", 6.0f}}),
            mkSlot (FxType::Reverb,          {{"size", 0.78f}, {"damp", 0.42f}, {"wet", -3.0f}}),
            mkSlot (FxType::Delay,           {{"time", 375.0f}, {"feedback", 0.42f}, {"wet", -10.0f}}),
            mkSlot (FxType::Octave,          {{"mix", -14.0f}}),
            mkSlot (FxType::Limiter,         {{"ceiling", -1.0f}, {"release", 80.0f}}),
        }},
        { u8"电台厚声", {
            mkSlot (FxType::SpectralDenoise, {{"strength", 0.5f}}),
            mkSlot (FxType::Highpass,        {{"freq", 85.0f}, {"q", 0.707f}}),
            mkSlot (FxType::Lowpass,         {{"freq", 11000.0f}, {"q", 0.707f}}),
            mkSlot (FxType::PeakEQ,          {{"freq", 180.0f}, {"q", 1.0f}, {"gain", 3.5f}}),
            mkSlot (FxType::PeakEQ,          {{"freq", 3000.0f}, {"q", 1.0f}, {"gain", 2.5f}}),
            mkSlot (FxType::Compressor,      {{"threshold", -22.0f}, {"ratio", 4.0f}, {"attack", 6.0f}, {"release", 90.0f}, {"knee", 4.0f}}),
            mkSlot (FxType::Limiter,         {{"ceiling", -0.8f}, {"release", 50.0f}}),
        }},
    };
}

// ---------------------------------------------------------------------------
// 机架：持有整条效果链 + 音频 IO + 电平统计
// ---------------------------------------------------------------------------
class Rack : public juce::AudioIODeviceCallback
{
public:
    Rack()
    {
        applyPreset (1); // 默认唱歌
    }

    ~Rack() override { stop(); }

    // ---------------- 设备枚举 ----------------
    static juce::StringArray listHostTypes()
    {
        juce::StringArray a;
        auto& types = juce::AudioIODeviceType::getAvailableDeviceTypes();
        for (auto* t : types)
            a.add (t->getTypeName());
        return a;
    }

    static juce::StringArray listDevices (const juce::String& hostType)
    {
        juce::StringArray out;
        auto& types = juce::AudioIODeviceType::getAvailableDeviceTypes();
        for (auto* t : types)
            if (t->getTypeName() == hostType)
            {
                t->scanForDevices();
                out = t->getDeviceNames (false);
                break;
            }
        return out;
    }

    static juce::StringArray listChannels (const juce::String& hostType, const juce::String& dev, bool input)
    {
        juce::StringArray out;
        auto& types = juce::AudioIODeviceType::getAvailableDeviceTypes();
        for (auto* t : types)
            if (t->getTypeName() == hostType)
            {
                t->scanForDevices();
                if (auto* d = t->createDevice (dev, dev))
                {
                    out = input ? d->getInputChannelNames() : d->getOutputChannelNames();
                }
                break;
            }
        return out;
    }

    // ---------------- 开流 ----------------
    juce::String start (const juce::String& hostType,
                        const juce::String& deviceName,
                        double sampleRate,
                        int blockSize)
    {
        stop();
        juce::String err;

        auto& types = juce::AudioIODeviceType::getAvailableDeviceTypes();
        juce::AudioIODeviceType* chosen = nullptr;
        for (auto* t : types)
            if (t->getTypeName() == hostType) { chosen = t; break; }

        if (chosen == nullptr && ! types.isEmpty())
            chosen = types[0];

        if (chosen == nullptr)
            return u8"找不到任何音频驱动";

        chosen->scanForDevices();
        std::unique_ptr<juce::AudioIODevice> dev (
            chosen->createDevice (deviceName, deviceName));
        if (dev == nullptr)
            return u8"打不开设备：" + deviceName;

        // 用设备支持的采样率 / 块大小，否则退回请求值
        auto srOk = dev->getAvailableSampleRates();     // Array<double>
        if (srOk.size() > 0)
        {
            bool found = false;
            for (auto v : srOk)
                if (std::abs (v - sampleRate) < 1.0) { found = true; break; }
            if (! found)
                sampleRate = srOk[0];
        }

        auto bsOk = dev->getAvailableBufferSizes();     // Array<int>
        if (bsOk.size() > 0)
        {
            bool found = false;
            for (auto v : bsOk)
                if (v == blockSize) { found = true; break; }
            if (! found)
                blockSize = bsOk[0];
        }

        err = dev->open (dev->getInputChannelNames(),
                         dev->getOutputChannelNames(),
                         sampleRate, blockSize);
        if (err.isNotEmpty())
            return err;

        currentDevice = std::move (dev);
        currentHost   = hostType;

        const auto sr = currentDevice->getCurrentSampleRate();
        const auto bs = currentDevice->getCurrentBufferSizeSamples();
        prepareChain (sr);

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

    juce::String deviceInfo() const
    {
        if (currentDevice == nullptr) return u8"未连接";
        return currentHost + " · " + currentDevice->getName()
             + juce::String (" · ") + juce::String (currentDevice->getCurrentSampleRate(), 0) + " Hz"
             + juce::String (" · ") + juce::String (currentDevice->getCurrentBufferSizeSamples()) + " smp";
    }

    // ---------------- 效果链 ----------------
    void applyPreset (int idx)
    {
        auto ps = presets();
        if (idx < 0 || idx >= (int) ps.size()) idx = 0;
        presetIndex = idx;
        slots = ps[(size_t) idx].chain;
        rebuild();
    }

    int getPresetIndex() const { return presetIndex; }
    const std::vector<FxSlot>& getSlots() const { return slots; }
    std::vector<FxSlot>& getSlotsMutable() { return slots; }

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
    }

    void setBypass (bool b) { bypassAll = b; }
    bool getBypass() const  { return bypassAll; }

    // 单声道电平（0..1）
    float getInLevel()  const { return inLevel.load(); }
    float getOutLevel() const { return outLevel.load(); }
    bool  getClipping() const { return clipping.load(); }
    double getBlockMs() const { return blockMs.load(); }
    int    getXruns()   const { return xruns.load(); }

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
        float*       dst = out[0];

        const juce::ScopedTryLock stl (chainLock);
        if (! stl.isLocked())
        {
            juce::FloatVectorOperations::copy (dst, src, numSamples);
            recordTime (t0);
            return;
        }

        float peakIn = 0.0f, peakOut = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            float x = src[i];
            peakIn = juce::jmax (peakIn, std::fabs (x));

            if (! bypassAll)
                for (auto& r : runtimes)
                    x = r.process (x);

            // 软限幅保护
            if (x > 1.0f)  x = 1.0f;
            if (x < -1.0f) x = -1.0f;

            dst[i] = x;
            peakOut = juce::jmax (peakOut, std::fabs (x));
        }

        inLevel.store (peakIn);
        outLevel.store (peakOut);
        if (peakOut >= 0.999f) clipping.store (true);
        recordTime (t0);
    }

    void audioDeviceAboutToStart (juce::AudioIODevice* d) override
    {
        currentSampleRate = d->getCurrentSampleRate();
        rebuild();
    }

    void audioDeviceStopped() override {}

private:
    void prepareChain (double sr)
    {
        currentSampleRate = sr;
        rebuild();
    }

    void recordTime (juce::int64 t0)
    {
        const auto t1 = juce::Time::getHighResolutionTicks();
        const double ms = juce::Time::highResolutionTicksToSeconds (t1 - t0) * 1000.0;
        blockMs.store (ms);
    }

    std::unique_ptr<juce::AudioIODevice> currentDevice;
    juce::String currentHost;
    double currentSampleRate = 48000.0;

    std::vector<FxSlot>    slots;
    std::vector<FxRuntime> runtimes;
    juce::CriticalSection  chainLock;
    int  presetIndex = 1;
    bool running = false;
    bool bypassAll = false;

    std::atomic<float>  inLevel  { 0.0f };
    std::atomic<float>  outLevel { 0.0f };
    std::atomic<bool>   clipping { false };
    std::atomic<double> blockMs  { 0.0 };
    std::atomic<int>    xruns    { 0 };
};

} // namespace miono
