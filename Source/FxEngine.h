// ============================================================================
//  MioNo - 引擎核心 / DSP
//  纯 C++ 重写，不依赖任何外部 DSP 库
//  对应 Python 版 rack_engine.py
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <vector>

namespace miono
{

// ---------------------------------------------------------------------------
// 效果类型 —— 与界面下拉框顺序一致
// ---------------------------------------------------------------------------
enum class FxType
{
    SpectralDenoise = 0,
    NoiseGate,
    Highpass,
    Lowpass,
    PeakEQ,
    Compressor,
    Reverb,
    Delay,
    Octave,
    Limiter
};

inline const char* fxName (FxType t)
{
    switch (t)
    {
        case FxType::SpectralDenoise: return u8"谱减降噪";
        case FxType::NoiseGate:       return u8"噪声门";
        case FxType::Highpass:        return u8"高通";
        case FxType::Lowpass:         return u8"低通";
        case FxType::PeakEQ:          return u8"峰值EQ";
        case FxType::Compressor:      return u8"压缩器";
        case FxType::Reverb:          return u8"混响";
        case FxType::Delay:           return u8"延迟";
        case FxType::Octave:          return u8"八度叠加";
        case FxType::Limiter:         return u8"限幅器";
        default:                      return u8"?";
    }
}

// ---------------------------------------------------------------------------
// 参数：一个效果槽 = 类型 + 开关 + 若干旋钮值
// ---------------------------------------------------------------------------
struct FxParam
{
    juce::String id;
    juce::String label;
    float        minVal  = 0.0f;
    float        maxVal  = 1.0f;
    float        defVal  = 0.0f;
    bool         isDb    = false;
    bool         isMs    = false;
    bool         isHz    = false;
    juce::String unit;
};

struct FxSlot
{
    FxType                 type = FxType::Highpass;
    bool                   on   = true;
    std::map<juce::String, float> params;
    // 运行期状态（不进参数面板）
    float get (const juce::String& k, float dflt = 0.0f) const
    {
        auto it = params.find (k);
        return it == params.end() ? dflt : it->second;
    }
    void set (const juce::String& k, float v) { params[k] = v; }
};

// ---------------------------------------------------------------------------
// RBJ 双二阶滤波器（高通 / 低通 / 峰值EQ）
// 与 Python 版 Biquad 完全一致的系数公式
// ---------------------------------------------------------------------------
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0;

    void reset() { x1 = x2 = y1 = y2 = 0; }

    void setHighpass (double sr, double f, double q)
    {
        const double w0    = 2.0 * juce::MathConstants<double>::pi * f / sr;
        const double cosw  = std::cos (w0);
        const double alpha = std::sin (w0) / (2.0 * q);
        const double a0    = 1.0 + alpha;
        b0 = (float) ((1.0 + cosw) / 2.0 / a0);
        b1 = (float) (-(1.0 + cosw) / a0);
        b2 = (float) ((1.0 + cosw) / 2.0 / a0);
        a1 = (float) (-2.0 * cosw / a0);
        a2 = (float) ((1.0 - alpha) / a0);
    }

    void setLowpass (double sr, double f, double q)
    {
        const double w0    = 2.0 * juce::MathConstants<double>::pi * f / sr;
        const double cosw  = std::cos (w0);
        const double alpha = std::sin (w0) / (2.0 * q);
        const double a0    = 1.0 + alpha;
        b0 = (float) ((1.0 - cosw) / 2.0 / a0);
        b1 = (float) ((1.0 - cosw) / a0);
        b2 = (float) ((1.0 - cosw) / 2.0 / a0);
        a1 = (float) (-2.0 * cosw / a0);
        a2 = (float) ((1.0 - alpha) / a0);
    }

    void setPeak (double sr, double f, double q, double gainDb)
    {
        const double A     = std::pow (10.0, gainDb / 40.0);
        const double w0    = 2.0 * juce::MathConstants<double>::pi * f / sr;
        const double cosw  = std::cos (w0);
        const double alpha = std::sin (w0) / (2.0 * q);
        const double a0    = 1.0 + alpha / A;
        b0 = (float) ((1.0 + alpha * A) / a0);
        b1 = (float) ((-2.0 * cosw) / a0);
        b2 = (float) ((1.0 - alpha * A) / a0);
        a1 = (float) ((-2.0 * cosw) / a0);
        a2 = (float) ((1.0 - alpha / A) / a0);
    }

    inline float process (float x) noexcept
    {
        const float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x;
        y2 = y1; y1 = y;
        return y;
    }
};

// ---------------------------------------------------------------------------
// 噪声门 —— 对应 Python 版 NoiseGateFx
// ---------------------------------------------------------------------------
struct NoiseGateFx
{
    float thresholdDb = -45.0f;
    float attackMs    = 3.0f;
    float releaseMs   = 120.0f;
    float env = 0.0f;
    double sr = 48000.0;
    float holdCounter = 0.0f;
    float holdMs = 30.0f;

    void reset() { env = 0.0f; holdCounter = 0.0f; }

    float process (float x) noexcept
    {
        const float a = std::fabs (x);
        const float atk = std::exp (-1.0f / (float) (sr * attackMs  / 1000.0));
        const float rel = std::exp (-1.0f / (float) (sr * releaseMs / 1000.0));
        env = (a > env) ? (atk * env + (1.0f - atk) * a)
                        : (rel * env + (1.0f - rel) * a);
        const float envDb = juce::Decibels::gainToDecibels (env, -100.0f);
        if (envDb > thresholdDb)
            holdCounter = (float) (sr * holdMs / 1000.0);
        else if (holdCounter > 0.0f)
            holdCounter -= 1.0f;

        const bool open = (envDb > thresholdDb) || (holdCounter > 0.0f);
        const float target = open ? 1.0f : 0.0f;
        static thread_local float g = 0.0f;
        const float coef = open ? 0.35f : 0.006f;
        g += (target - g) * coef;
        return x * g;
    }
};

// ---------------------------------------------------------------------------
// 压缩器 —— 对应 Python 版 CompressorFx
// ---------------------------------------------------------------------------
struct CompressorFx
{
    float thresholdDb = -18.0f;
    float ratio       = 3.0f;
    float attackMs    = 10.0f;
    float releaseMs   = 100.0f;
    float kneeDb      = 6.0f;
    float makeupDb    = 0.0f;
    double sr = 48000.0;
    float env = 0.0f;

    void reset() { env = 0.0f; }

    float process (float x) noexcept
    {
        const float a = std::fabs (x);
        const float atk = std::exp (-1.0f / (float) (sr * attackMs  / 1000.0));
        const float rel = std::exp (-1.0f / (float) (sr * releaseMs / 1000.0));
        env = (a > env) ? (atk * env + (1.0f - atk) * a)
                        : (rel * env + (1.0f - rel) * a);

        const float envDb = juce::Decibels::gainToDecibels (env, -100.0f);
        float over = envDb - thresholdDb;
        float gainDb = 0.0f;

        if (over > 0.0f)
        {
            if (kneeDb > 0.0f && over < kneeDb)
            {
                // 软拐点
                const float t = over / kneeDb;
                gainDb = -(over * over) / (2.0f * kneeDb) * (1.0f - 1.0f / ratio) * 2.0f;
                juce::ignoreUnused (t);
            }
            else
            {
                gainDb = -over * (1.0f - 1.0f / ratio);
            }
        }
        const float makeup = makeupDb + (-thresholdDb) * 0.35f;
        return x * juce::Decibels::decibelsToGain (gainDb + makeup);
    }
};

// ---------------------------------------------------------------------------
// 限幅器 —— 对应 Python 版 LimiterFx（砖墙 + 前视简化）
// ---------------------------------------------------------------------------
struct LimiterFx
{
    float ceilingDb = -1.0f;
    float releaseMs = 60.0f;
    double sr = 48000.0;
    float env = 0.0f;

    void reset() { env = 0.0f; }

    float process (float x) noexcept
    {
        const float ceil = juce::Decibels::decibelsToGain (ceilingDb);
        const float a = std::fabs (x);
        const float rel = std::exp (-1.0f / (float) (sr * releaseMs / 1000.0));
        env = (a > env) ? a : (rel * env + (1.0f - rel) * a);
        if (env > ceil && env > 1e-9f)
        {
            const float g = ceil / env;
            return x * g;
        }
        return x;
    }
};

// ---------------------------------------------------------------------------
// 延迟 —— 对应 Python 版 DelayFx（环形缓冲 + 反馈 + 干湿）
// ---------------------------------------------------------------------------
struct DelayFx
{
    std::vector<float> buf;
    int   writePos = 0;
    float delayMs  = 250.0f;
    float feedback = 0.3f;
    float wetDb    = -10.0f;
    double sr = 48000.0;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        buf.assign ((size_t) (sampleRate * 2.5), 0.0f);
        writePos = 0;
    }

    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); writePos = 0; }

    float process (float x) noexcept
    {
        if (buf.empty()) return x;
        const int n = (int) buf.size();
        const int d = juce::jlimit (1, n - 1, (int) (sr * delayMs / 1000.0));
        int rp = writePos - d;
        if (rp < 0) rp += n;
        const float delayed = buf[(size_t) rp];
        buf[(size_t) writePos] = x + delayed * feedback;
        if (++writePos >= n) writePos = 0;
        const float wet = juce::Decibels::decibelsToGain (wetDb);
        return x + delayed * wet;
    }
};

// ---------------------------------------------------------------------------
// 混响 —— Freeverb 思路（8 梳状 + 4 全通），对应 Python 版 ReverbFx
// ---------------------------------------------------------------------------
struct ReverbFx
{
    struct Comb
    {
        std::vector<float> buf;
        int   pos = 0;
        float store = 0.0f;
        float feedback = 0.84f;
        float damp = 0.2f;

        void init (int n) { buf.assign ((size_t) n, 0.0f); pos = 0; store = 0.0f; }
        float process (float x) noexcept
        {
            if (buf.empty()) return x;
            const float out = buf[(size_t) pos];
            store = out * (1.0f - damp) + store * damp;
            buf[(size_t) pos] = x + store * feedback;
            if (++pos >= (int) buf.size()) pos = 0;
            return out;
        }
    };
    struct Allpass
    {
        std::vector<float> buf;
        int   pos = 0;
        float feedback = 0.5f;

        void init (int n) { buf.assign ((size_t) n, 0.0f); pos = 0; }
        float process (float x) noexcept
        {
            if (buf.empty()) return x;
            const float bufout = buf[(size_t) pos];
            const float out = -x + bufout;
            buf[(size_t) pos] = x + bufout * feedback;
            if (++pos >= (int) buf.size()) pos = 0;
            return out;
        }
    };

    static constexpr int kCombs = 8;
    static constexpr int kAlls  = 4;
    std::array<Comb, kCombs>    combs;
    std::array<Allpass, kAlls>  alls;

    static constexpr int combTune[kCombs]  = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    static constexpr int allTune[kAlls]    = { 556, 441, 341, 225 };

    float roomSize = 0.5f;
    float damping  = 0.5f;
    float wetDb    = -6.0f;
    double sr = 48000.0;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        const double scale = sampleRate / 44100.0;
        for (int i = 0; i < kCombs; ++i)
            combs[(size_t) i].init (juce::jmax (8, (int) (combTune[i] * scale)));
        for (int i = 0; i < kAlls; ++i)
            alls[(size_t) i].init (juce::jmax (8, (int) (allTune[i] * scale)));
    }

    void reset()
    {
        for (auto& c : combs) { std::fill (c.buf.begin(), c.buf.end(), 0.0f); c.pos = 0; c.store = 0.0f; }
        for (auto& a : alls)  { std::fill (a.buf.begin(),  a.buf.end(),  0.0f); a.pos = 0; }
    }

    float process (float x) noexcept
    {
        const float fb = 0.7f + roomSize * 0.28f;
        const float dp = damping;
        float out = 0.0f;
        for (auto& c : combs)
        {
            c.feedback = fb;
            c.damp = dp;
            out += c.process (x);
        }
        out *= 1.0f / (float) kCombs;
        for (auto& a : alls)
            out = a.process (out);
        const float wet = juce::Decibels::decibelsToGain (wetDb);
        return x + out * wet * 3.0f;
    }
};

// ---------------------------------------------------------------------------
// 八度叠加 —— 对应 Python 版 OctaveFx（简单倍频 + 混音）
// ---------------------------------------------------------------------------
struct OctaveFx
{
    float mixDb = -12.0f;
    float prev = 0.0f;

    void reset() { prev = 0.0f; }

    float process (float x) noexcept
    {
        // 平方产生倍频分量（简易 octaver，与 Python 版思路一致）
        const float sq = x * x * (x >= 0.0f ? 1.0f : -1.0f);
        prev = sq;
        const float mix = juce::Decibels::decibelsToGain (mixDb);
        return x + prev * mix;
    }
};

// ---------------------------------------------------------------------------
// 谱减降噪 —— 对应 Python 版 SpectralDenoiseFx
// STFT + 维纳增益谱减，核心算法逐行对齐
// ---------------------------------------------------------------------------
class SpectralDenoiseFx
{
public:
    void prepare (double sampleRate, int fftSize = 1024, int hop = 256)
    {
        sr      = sampleRate;
        nfft    = fftSize;
        hopSize = hop;
        fft     = std::make_unique<juce::dsp::FFT> ((int) std::log2 (nfft));
        window  = std::make_unique<juce::dsp::WindowingFunction<float>> (
                      (size_t) nfft, juce::dsp::WindowingFunction<float>::hann, true);
        noiseMag.assign ((size_t) (nfft / 2 + 1), 0.0f);
        spec.assign ((size_t) nfft * 2, 0.0f);
        frame.assign ((size_t) nfft, 0.0f);
        olaBuf.assign ((size_t) nfft * 4, 0.0f);
        olaCnt.assign ((size_t) nfft * 4, 0.0f);
        inPos = 0; outPos = 0;
        primed = false;
    }

    void reset()
    {
        std::fill (noiseMag.begin(), noiseMag.end(), 0.0f);
        std::fill (spec.begin(), spec.end(), 0.0f);
        std::fill (frame.begin(), frame.end(), 0.0f);
        std::fill (olaBuf.begin(), olaBuf.end(), 0.0f);
        std::fill (olaCnt.begin(), olaCnt.end(), 0.0f);
        inPos = 0; outPos = 0; primed = false;
    }

    // strength 0..1
    void setStrength (float s) { strength = juce::jlimit (0.0f, 1.0f, s); }

    float process (float x) noexcept
    {
        if (fft == nullptr) return x;
        inBuf[(size_t) inPos] = x;
        if (++inPos >= hopSize)
        {
            inPos = 0;
            processFrame();
        }
        float out = 0.0f;
        if (outPos < (int) olaBuf.size())
        {
            const float c = olaCnt[(size_t) outPos];
            out = (c > 1e-6f) ? olaBuf[(size_t) outPos] / c : 0.0f;
        }
        // 环形推进输出
        outPos = (outPos + 1) % (int) olaBuf.size();
        return out;
    }

private:
    void processFrame() noexcept
    {
        // 组装一帧：把 inBuf 的内容追加到 frame 尾部，前部保留上一帧历史
        std::memmove (frame.data(), frame.data() + hopSize,
                      sizeof (float) * (size_t) (nfft - hopSize));
        std::memcpy (frame.data() + (nfft - hopSize), inBuf.data(),
                     sizeof (float) * (size_t) hopSize);

        // ---- 正变换：JUCE 8 公开 API，实部虚部交错存放 ----
        std::fill (spec.begin(), spec.end(), 0.0f);
        std::memcpy (spec.data(), frame.data(), sizeof (float) * (size_t) nfft);
        window->multiplyWithWindowingTable (spec.data(), (size_t) nfft);
        fft->perform (spec.data(), spec.data(), false);   // false = 正变换

        const int bins = nfft / 2 + 1;
        for (int k = 0; k < bins; ++k)
        {
            const float re = spec[(size_t) (2 * k)];
            const float im = spec[(size_t) (2 * k + 1)];
            const float mag = std::sqrt (re * re + im * im);

            // 非对称平滑（对应 Python：d>0 用 0.0015，否则 0.15）
            const float d = mag - noiseMag[(size_t) k];
            noiseMag[(size_t) k] += (d > 0.0f) ? (0.0015f * d) : (0.15f * d);

            // 偏置补偿
            const float est = noiseMag[(size_t) k] * (2.5f + strength * 1.6f);
            float snr = mag / juce::jmax (est, 1e-9f) - 1.0f;
            if (snr < 0.0f) snr = 0.0f;
            float gain = snr / (snr + 1.0f);
            gain = juce::jlimit (0.05f, 1.0f, gain);

            spec[(size_t) (2 * k)]     = re * gain;
            spec[(size_t) (2 * k + 1)] = im * gain;
        }

        // ---- 逆变换：true = 逆变换；结果落在 spec 前 nfft 个 float ----
        fft->perform (spec.data(), spec.data(), true);
        window->multiplyWithWindowingTable (spec.data(), (size_t) nfft);

        // 重叠相加
        const int sz = (int) olaBuf.size();
        for (int i = 0; i < nfft; ++i)
        {
            const int idx = (outPos + i) % sz;
            olaBuf[(size_t) idx] += spec[(size_t) i];
            olaCnt[(size_t) idx] += 1.0f;
        }
        for (int i = 0; i < hopSize; ++i)
        {
            const int idx = (outPos + i) % sz;
            if (olaCnt[(size_t) idx] > 1.0f)
            {
                olaBuf[(size_t) idx] /= olaCnt[(size_t) idx];
                olaCnt[(size_t) idx] = 1.0f;
            }
        }
    }

    double sr = 48000.0;
    int    nfft = 1024;
    int    hopSize = 256;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
    std::vector<float> noiseMag, spec, frame, olaBuf, olaCnt;
    std::array<float, 512> inBuf {};
    int inPos = 0, outPos = 0;
    bool primed = false;
    float strength = 0.5f;
};

// ---------------------------------------------------------------------------
// 一个效果槽的运行时容器
// ---------------------------------------------------------------------------
struct FxRuntime
{
    FxType type = FxType::Highpass;
    bool   on   = true;

    Biquad          biquad;
    NoiseGateFx     gate;
    CompressorFx    comp;
    LimiterFx       limiter;
    DelayFx         delay;
    ReverbFx        reverb;
    OctaveFx        octave;
    SpectralDenoiseFx denoise;

    void prepare (double sr)
    {
        gate.sr = sr;
        comp.sr = sr;
        limiter.sr = sr;
        delay.prepare (sr);
        reverb.prepare (sr);
        denoise.prepare (sr);
        biquad.reset();
        gate.reset();
        comp.reset();
        limiter.reset();
        delay.reset();
        reverb.reset();
        octave.reset();
        denoise.reset();
    }

    void configure (const FxSlot& s)
    {
        type = s.type;
        on   = s.on;
        switch (type)
        {
            case FxType::Highpass:
                biquad.setHighpass (48000.0, s.get ("freq", 90.0f), s.get ("q", 0.707f));
                break;
            case FxType::Lowpass:
                biquad.setLowpass (48000.0, s.get ("freq", 12000.0f), s.get ("q", 0.707f));
                break;
            case FxType::PeakEQ:
                biquad.setPeak (48000.0, s.get ("freq", 1000.0f), s.get ("q", 1.0f), s.get ("gain", 0.0f));
                break;
            case FxType::NoiseGate:
                gate.thresholdDb = s.get ("threshold", -45.0f);
                gate.attackMs    = s.get ("attack", 3.0f);
                gate.releaseMs   = s.get ("release", 120.0f);
                break;
            case FxType::Compressor:
                comp.thresholdDb = s.get ("threshold", -18.0f);
                comp.ratio       = s.get ("ratio", 3.0f);
                comp.attackMs    = s.get ("attack", 10.0f);
                comp.releaseMs   = s.get ("release", 100.0f);
                comp.kneeDb      = s.get ("knee", 6.0f);
                comp.makeupDb    = s.get ("makeup", 0.0f);
                break;
            case FxType::Limiter:
                limiter.ceilingDb = s.get ("ceiling", -1.0f);
                limiter.releaseMs = s.get ("release", 60.0f);
                break;
            case FxType::Delay:
                delay.delayMs  = s.get ("time", 250.0f);
                delay.feedback = s.get ("feedback", 0.3f);
                delay.wetDb    = s.get ("wet", -10.0f);
                break;
            case FxType::Reverb:
                reverb.roomSize = s.get ("size", 0.5f);
                reverb.damping  = s.get ("damp", 0.5f);
                reverb.wetDb    = s.get ("wet", -6.0f);
                break;
            case FxType::Octave:
                octave.mixDb = s.get ("mix", -12.0f);
                break;
            case FxType::SpectralDenoise:
                denoise.setStrength (s.get ("strength", 0.5f));
                break;
            default: break;
        }
    }

    inline float process (float x) noexcept
    {
        if (! on) return x;
        switch (type)
        {
            case FxType::Highpass:
            case FxType::Lowpass:
            case FxType::PeakEQ:          return biquad.process (x);
            case FxType::NoiseGate:       return gate.process (x);
            case FxType::Compressor:      return comp.process (x);
            case FxType::Limiter:         return limiter.process (x);
            case FxType::Delay:           return delay.process (x);
            case FxType::Reverb:          return reverb.process (x);
            case FxType::Octave:          return octave.process (x);
            case FxType::SpectralDenoise: return denoise.process (x);
            default:                      return x;
        }
    }
};

} // namespace miono
