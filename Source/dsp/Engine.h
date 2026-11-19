// Deriva — prime-number memory drifter
// DSP engine (header-only, no JUCE dependency so it can be unit-tested offline).
//
// Algorithms / references
//  * Shared memory pool (44.7 s) split into delay lines whose lengths are distinct primes,
//    so repetitions never realign (SOMA COSMOS manual v1.3, "Concept").
//  * Each line is a tape-loop head: a single fractional pointer reads and writes
//    (Rocchesso, "Fractionally-addressed delay lines", IEEE TSAP 2000 / DAFx-98).
//    Reads use 4-point 3rd-order Hermite interpolation (Niemitalo, "Polynomial
//    interpolators for high-quality resampling of oversampled audio", 2001).
//  * BLUR = orthogonal (energy-preserving) feedback mixing built from Givens rotations,
//    which at 45 deg equals a normalised Hadamard (Gerzon 1976; Jot & Chaigne 1991;
//    Smith, "Physical Audio Signal Processing"; Luff, "Let's write a reverb", 2021).
//  * Giant reverb = 8-line FDN with a Householder matrix (low diffusion, so early
//    reflections stay audible as discrete echoes) plus lossless Schroeder allpasses
//    in each loop that make the echoes merge slowly into diffuse noise.
//  * Filters: TPT / zero-delay-feedback state-variable filter
//    (Zavalishin, "The Art of VA Filter Design" 2.1; Simper, "Linear trap SVF").
//  * DRIVE: tanh waveshaper with 1st-order antiderivative anti-aliasing
//    (Parker, Zavalishin & Le Bivic, DAFx-16; Bilbao et al., IEEE SPL 2017).
//  * DRIFT: per-line asynchronous LFOs with irrational rate ratios, rate modulated by
//    a slow Lorenz attractor (chaotic control signal).
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace deriva
{
constexpr double kMemorySeconds = 44.7;
constexpr int kMaxLines = 8;
constexpr int kEnvBins = 160;
constexpr int kMaxGrains = 10;
constexpr int kControlRate = 16;
constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

enum class AlgoType { TwoDelays, FourDelays, Reverb, Granular };

inline int clampAlgo (int a) { return std::clamp (a, 1, 12); }
inline AlgoType algoType (int a)
{
    a = clampAlgo (a);
    if (a <= 3) return AlgoType::TwoDelays;
    if (a <= 6) return AlgoType::FourDelays;
    if (a <= 9) return AlgoType::Reverb;
    return AlgoType::Granular;
}
inline int algoVariant (int a) { return (clampAlgo (a) - 1) % 3; }

inline bool isPrime (int64_t n)
{
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;
    for (int64_t d = 3; d * d <= n; d += 2)
        if (n % d == 0) return false;
    return true;
}
inline int nextPrime (int64_t n)
{
    if (n <= 2) return 2;
    if (n % 2 == 0) ++n;
    while (! isPrime (n)) n += 2;
    return (int) n;
}

//==============================================================================
struct Smoother
{
    float a = 0.0f, z = 0.0f;
    void setTime (float seconds, float sr) { a = std::exp (-1.0f / (std::max (1.0e-5f, seconds) * sr)); }
    void reset (float v) { z = v; }
    float next (float target) { z = target + a * (z - target); return z; }
};

struct Rng
{
    uint32_t s = 0x9E3779B9u;
    uint32_t nextU() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() { return (float) (nextU() >> 8) * (1.0f / 16777216.0f); } // [0,1)
    float bi() { return uni() * 2.0f - 1.0f; }                              // [-1,1)
};

// Zero-delay-feedback state variable filter (Zavalishin / Simper)
struct SVF
{
    float g = 0, k = 1.41421356f, a1 = 0, a2 = 0, a3 = 0, ic1 = 0, ic2 = 0;
    void set (float fc, float sr, float q = 0.70710678f)
    {
        fc = std::clamp (fc, 5.0f, 0.45f * sr);
        g = std::tan (kPi * fc / sr);
        k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    inline void process (float v0, float& lp, float& hp)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2;
        hp = v0 - k * v1 - v2;
    }
    void reset() { ic1 = ic2 = 0.0f; }
};

// Lossless Schroeder allpass used as in-loop diffuser for the giant reverb
struct Allpass
{
    std::vector<float> buf;
    int idx = 0, d = 1;
    float g = 0.6f;
    void allocate (int maxLen) { buf.assign ((size_t) std::max (2, maxLen), 0.0f); idx = 0; }
    void setDelay (int samples) { d = std::clamp (samples, 1, (int) buf.size() - 1); }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); idx = 0; }
    inline float process (float x)
    {
        const int n = (int) buf.size();
        int r = idx - d;
        if (r < 0) r += n;
        const float dl = buf[(size_t) r];
        const float w = x - g * dl;
        buf[(size_t) idx] = w;
        if (++idx >= n) idx = 0;
        return dl + g * w;
    }
};

// tanh drive with first-order antiderivative anti-aliasing
struct AdaaDrive
{
    float xPrev = 0.0f;
    static inline float logcosh (float x)
    {
        const float ax = std::fabs (x);
        return ax + std::log1p (std::exp (-2.0f * ax)) - 0.69314718f;
    }
    // f(u) = tanh(u + b) - tanh(b) ; F(u) = logcosh(u + b) - tanh(b) * u
    inline float process (float x, float k, float b, float tb, float norm)
    {
        const float u = k * x, up = k * xPrev;
        xPrev = x;
        const float du = u - up;
        float y;
        if (std::fabs (du) < 1.0e-4f)
            y = std::tanh (0.5f * (u + up) + b) - tb;
        else
            y = ((logcosh (u + b) - tb * u) - (logcosh (up + b) - tb * up)) / du;
        return y * norm;
    }
};

// Soft "tape-like" limiter: linear up to 0.5, tanh knee to a ceiling of 1.0
inline float softLimit (float x)
{
    const float ax = std::fabs (x);
    if (ax <= 0.5f) return x;
    const float y = 0.5f + 0.5f * std::tanh ((ax - 0.5f) * 2.0f);
    return x < 0.0f ? -y : y;
}

// Output-bus safety clip: transparent below 0.8, smooth knee to 1.0
inline float busSafety (float x)
{
    const float ax = std::fabs (x);
    if (ax <= 0.8f) return x;
    const float y = 0.8f + 0.2f * std::tanh ((ax - 0.8f) * 5.0f);
    return x < 0.0f ? -y : y;
}

inline float hermite (float xm1, float x0, float x1, float x2, float t)
{
    const float c1 = 0.5f * (x1 - xm1);
    const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
    const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
    return ((c3 * t + c2) * t + c1) * t + x0;
}

//==============================================================================
struct Line
{
    int base = 0, len = 1;
    double p = 0.0;
    float acc = 0.0f;
    int cnt = 0;
    int group = 0;        // 0 = input L / record group 1, 1 = input R / group 2, 2 = centre (both)
    float panBase = 0.0f;
    float out = 0.0f;
    float panL = 0.7071f, panR = 0.7071f;
    float recSm = 1.0f;
    SVF hp, lp;
    float dcx = 0.0f, dcy = 0.0f;
    float env[kEnvBins] {};
    int curBin = -1;
};

struct Grain
{
    bool active = false;
    float age = 0.0f, len = 1.0f, delay = 0.0f, gl = 0.0f, gr = 0.0f;
};

//==============================================================================
class Engine
{
public:
    struct Params
    {
        int algo = 2;
        float inGain = 1.0f, blur = 0.0f, drift = 0.3f, drive = 0.0f, supcom = 0.0f, fb = 0.97f, mix = 0.5f, outGain = 1.0f;
        bool hpfOn = false; int hpfFreq = 0;
        bool lpfOn = false; int lpfFreq = 0;
        bool recOn = true; int recTarget = 2; // 0 = group 1, 1 = group 2, 2 = all
        bool reverse = false, halfSpeed = false, erase = false;
        bool monoInput = false, gateSensitive = false;
    };

    // Optional hooks used by the Beads stage (all pointers may be null)
    struct ExternalIO
    {
        float* wetL = nullptr;        // if set: raw memory output is written here and NOT mixed into out
        float* wetR = nullptr;
        float* wetGain = nullptr;     // per-sample gain the caller must apply to the (processed) wet
        const float* retL = nullptr;  // loop return (stereo), read per sample
        const float* retR = nullptr;
        float loopMix = 0.0f;         // 0 = clean feedback, 1 = feedback entirely through the return
    };

    static constexpr float kHpfFreqs[3] = { 60.0f, 150.0f, 400.0f };
    static constexpr float kLpfFreqs[3] = { 1500.0f, 4000.0f, 9000.0f }; // "min" = darkest

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        poolSize = (int) std::ceil (kMemorySeconds * sampleRate) + 64;
        pool.assign ((size_t) poolSize, 0.0f);

        for (auto& ap : allpasses)
            for (auto& a : ap) a.allocate ((int) (0.2f * sr));

        for (auto s : { &smIn, &smBlur, &smDrift, &smDrive, &smSup, &smFb, &smMix, &smOut })
            s->setTime (0.02f, sr);
        smIn.reset (1.0f); smBlur.reset (0.0f); smDrift.reset (0.3f); smDrive.reset (0.0f);
        smSup.reset (0.0f); smFb.reset (0.97f); smMix.reset (0.5f); smOut.reset (1.0f);

        dcR = 1.0f - kTwoPi * 5.0f / sr;
        hpMix = lpMix = 0.0f;
        hpFc = kHpfFreqs[0]; lpFc = kLpfFreqs[2];

        gateCoefRel = std::exp (-1.0f / (0.05f * sr));
        gateAtt = 1.0f / (0.001f * sr);
        gateRel = 1.0f / (0.2f * sr);
        holdSamples = (int) (0.05f * sr);
        supAtt = std::exp (-1.0f / (0.005f * sr));
        supRel = std::exp (-1.0f / (0.3f * sr));
        compRms = std::exp (-1.0f / (0.03f * sr));
        compAtt = std::exp (-1.0f / (0.01f * sr));
        compRel = std::exp (-1.0f / (0.25f * sr));

        for (int i = 0; i < kMaxLines; ++i)
            lfoPhase[(size_t) i] = (double) rng.uni() * kTwoPi;
        lorenzX = 1.0; lorenzY = 1.0; lorenzZ = 20.0;

        currentAlgo = targetAlgo = 2;
        configure (currentAlgo, true);
        tState = TState::Idle;
        tGain = 1.0f;
        speed = 1.0f;
    }

    void requestEraseAll() { eraseAllRequested = true; }

    // ---- GUI / host accessors (benign races, read-only) ----
    int numLinesNow() const { return numLines; }
    int currentAlgorithm() const { return currentAlgo; }
    float lineLengthSeconds (int i) const { return (float) lines[(size_t) i].len / sr; }
    float headFraction (int i) const { const auto& l = lines[(size_t) i]; return (float) (l.p / (double) l.len); }
    const float* lineEnvelope (int i) const { return lines[(size_t) i].env; }
    float lastLoopPeak() const { return meterPeak; }
    bool limiterWasActive() const { return limiterFlag; }
    bool inputClipped() const { return inClipFlag; }
    float lastInputPeak() const { return inPeak; }
    float speedNow() const { return speed; }
    float sampleRate() const { return sr; }

    std::vector<float>& memory() { return pool; }
    int memorySize() const { return poolSize; }
    double headPosition (int i) const { return lines[(size_t) i].p; }

    // Called from the audio thread with fresh pool content (state restore)
    void memoryWasReplaced()
    {
        for (int i = 0; i < numLines; ++i) rebuildEnvelope (lines[(size_t) i]);
    }

    //==========================================================================
    void process (const float* inL, const float* inR, float* outL, float* outR, int n, const Params& prm,
                  const ExternalIO* ext = nullptr)
    {
        const bool splitWet = ext != nullptr && ext->wetL != nullptr && ext->wetR != nullptr && ext->wetGain != nullptr;
        const bool loopReturn = ext != nullptr && ext->retL != nullptr && ext->retR != nullptr && ext->loopMix > 0.0f;
        const float loopMix = loopReturn ? std::clamp (ext->loopMix, 0.0f, 1.0f) : 0.0f;
        float blockPeak = 0.0f, blockIn = 0.0f;
        bool limiter = false, clip = false;

        const int wantAlgo = clampAlgo (prm.algo);
        if (wantAlgo != targetAlgo) targetAlgo = wantAlgo;
        if (eraseAllRequested) { eraseAllRequested = false; eraseAllPending = true; }

        const AlgoType typeNow = algoType (currentAlgo);
        const float speedTarget = (prm.halfSpeed && (typeNow == AlgoType::TwoDelays || typeNow == AlgoType::FourDelays)) ? 0.5f : 1.0f;
        const float speedStep = 0.5f / (0.15f * sr);
        const float dir = prm.reverse ? -1.0f : 1.0f;
        const float tStepAlgo = 1.0f / (0.03f * sr);
        const float tStepErase = 1.0f / (0.01f * sr);
        const float eraseStep = 1.0f / (0.01f * sr);
        const float gateThr = prm.gateSensitive ? 1.0e-4f : 1.0e-3f;
        const float recCoef = std::exp (-1.0f / (0.01f * sr));

        // continue a chunked "erase all"
        if (clearing)
        {
            const int chunk = 1 << 18;
            const int end = std::min (poolSize, clearPos + chunk);
            std::fill (pool.begin() + clearPos, pool.begin() + end, 0.0f);
            clearPos = end;
            if (clearPos >= poolSize)
            {
                clearing = false;
                for (int i = 0; i < kMaxLines; ++i)
                {
                    auto& l = lines[(size_t) i];
                    std::fill (std::begin (l.env), std::end (l.env), 0.0f);
                    l.hp.reset(); l.lp.reset(); l.dcx = l.dcy = 0.0f; l.acc = 0.0f; l.cnt = 0;
                    for (auto& a : allpasses[(size_t) i]) a.reset();
                    for (auto& g : grains[(size_t) i]) g.active = false;
                }
                tState = TState::FadeIn;
            }
        }

        for (int s = 0; s < n; ++s)
        {
            // ---------------- transition state machine
            if (tState == TState::Idle)
            {
                if (eraseAllPending) { tState = TState::FadeOutErase; eraseAllPending = false; }
                else if (targetAlgo != currentAlgo) tState = TState::FadeOut;
            }
            if (tState == TState::FadeOut)
            {
                tGain -= tStepAlgo;
                if (tGain <= 0.0f)
                {
                    tGain = 0.0f;
                    currentAlgo = targetAlgo;
                    configure (currentAlgo, false);
                    tState = TState::FadeIn;
                }
            }
            else if (tState == TState::FadeOutErase)
            {
                tGain -= tStepErase;
                if (tGain <= 0.0f) { tGain = 0.0f; tState = TState::Clearing; clearing = true; clearPos = 0; }
            }
            else if (tState == TState::FadeIn)
            {
                tGain += tStepAlgo;
                if (tGain >= 1.0f) { tGain = 1.0f; tState = TState::Idle; }
            }
            const bool writeEnabled = (tState == TState::Idle || tState == TState::FadeIn);
            const AlgoType type = algoType (currentAlgo);

            // ---------------- smoothed parameters
            const float inG = smIn.next (prm.inGain);
            const float blur = smBlur.next (prm.blur);
            const float drift = smDrift.next (prm.drift);
            const float drv = smDrive.next (prm.drive);
            const float supcom = smSup.next (prm.supcom);
            const float fb = smFb.next (prm.fb);
            const float mix = smMix.next (prm.mix);
            const float outG = smOut.next (prm.outGain);

            if (speed < speedTarget) speed = std::min (speedTarget, speed + speedStep);
            else if (speed > speedTarget) speed = std::max (speedTarget, speed - speedStep);
            if (speed == 1.0f && prevSpeed != 1.0f)
            {
                // back at normal speed: snap heads to integer cells (sub-sample jump, inaudible)
                for (int i = 0; i < numLines; ++i)
                {
                    auto& l = lines[(size_t) i];
                    l.p = std::fmod (std::round (l.p), (double) l.len);
                    l.acc = 0.0f; l.cnt = 0;
                }
            }
            prevSpeed = speed;

            eraseRamp = prm.erase ? std::min (1.0f, eraseRamp + eraseStep) : std::max (0.0f, eraseRamp - eraseStep);

            // ---------------- control rate
            if (--ctrlCounter <= 0)
            {
                ctrlCounter = kControlRate;
                controlUpdate (prm, drv, drift, type);
            }

            // ---------------- input stage
            float xL = inL[s] * inG;
            float xR = (inR != nullptr ? inR[s] : inL[s]) * inG;
            if (prm.monoInput) { const float m = 0.5f * (xL + xR); xL = xR = m; }
            blockIn = std::max (blockIn, std::max (std::fabs (xL), std::fabs (xR)));
            if (blockIn > 0.99f) clip = true;
            const float dryL = xL, dryR = xR;

            float gL = gate (0, xL, gateThr);
            float gR = gate (1, xR, gateThr);
            if (drv > 0.001f)
            {
                gL = drvL.process (gL, drvK, drvB, drvTB, drvNorm);
                gR = drvR.process (gR, drvK, drvB, drvTB, drvNorm);
            }
            else { drvL.xPrev = gL; drvR.xPrev = gR; }

            const float aIn = std::max (std::fabs (gL), std::fabs (gR));
            supEnv = aIn > supEnv ? aIn + supAtt * (supEnv - aIn) : aIn + supRel * (supEnv - aIn);

            // ---------------- read memory
            float wetL = 0.0f, wetR = 0.0f;
            float outs[kMaxLines] {};
            if (type == AlgoType::Granular)
            {
                for (int i = 0; i < numLines; ++i)
                    outs[i] = granularRead (i, dir, wetL, wetR);
            }
            else
            {
                for (int i = 0; i < numLines; ++i)
                {
                    auto& l = lines[(size_t) i];
                    l.out = readAt (l, l.p);
                    outs[i] = l.out;
                    wetL += l.out * l.panL;
                    wetR += l.out * l.panR;
                }
                wetL *= wetNorm; wetR *= wetNorm;
            }
            wetL = busSafety (wetL);
            wetR = busSafety (wetR);

            // ---------------- feedback mixing (BLUR / FDN)
            float mixed[kMaxLines];
            mixLines (outs, mixed, type, blur);

            // ---------------- dynamics: suppressor (feedback) and compressor (write sum)
            float gSup = 1.0f;
            if (supcom < -0.02f)
            {
                const float a = std::min (1.0f, (-supcom - 0.02f) / 0.98f);
                gSup = 1.0f - a * std::min (1.0f, supEnv / 0.125f);
            }
            float gComp = 1.0f;
            if (supcom > 0.02f)
            {
                const float c = std::min (1.0f, (supcom - 0.02f) / 0.98f);
                float memAbs = 0.0f;
                for (int i = 0; i < numLines; ++i) memAbs += std::fabs (outs[i]);
                memAbs /= (float) std::max (1, numLines);
                const float det = 0.5f * (std::fabs (gL) + std::fabs (gR)) + memAbs;
                compPow = det * det + compRms * (compPow - det * det);
                const float lvl = 10.0f * std::log10 (compPow + 1.0e-12f);
                const float ratio = 1.0f + 3.0f * c, thr = -24.0f, knee = 6.0f;
                const float over = lvl - thr;
                float red = 0.0f;
                if (over > knee * 0.5f) red = over * (1.0f - 1.0f / ratio);
                else if (over > -knee * 0.5f) { const float t = over + knee * 0.5f; red = (1.0f - 1.0f / ratio) * t * t / (2.0f * knee); }
                const float makeup = c * std::min (6.0f, -thr * (1.0f - 1.0f / ratio) * 0.25f);
                const float target = makeup - red;
                compGainDb = target < compGainDb ? target + compAtt * (compGainDb - target) : target + compRel * (compGainDb - target);
                gComp = std::pow (10.0f, compGainDb * 0.05f);
            }
            else compGainDb = 0.0f;

            const float fbGain = fb * gSup;
            const float inputMid = 0.5f * (gL + gR);
            float retL = 0.0f, retR = 0.0f;
            if (loopReturn) { retL = ext->retL[s]; retR = ext->retR[s]; }

            // ---------------- write
            for (int i = 0; i < numLines; ++i)
            {
                auto& l = lines[(size_t) i];
                // DC blocker
                float v = mixed[i];
                if (loopReturn)
                {
                    const float r = l.group == 0 ? retL : (l.group == 1 ? retR : 0.5f * (retL + retR));
                    v += loopMix * (r * groupNorm[(size_t) std::min (l.group, 2)] - v);
                }
                const float dcOut = v - l.dcx + dcR * l.dcy;
                l.dcx = v; l.dcy = dcOut; v = dcOut;
                // filters with crossfade
                if (hpMix > 0.0f || prm.hpfOn) { float lo, hi; l.hp.process (v, lo, hi); v += hpMix * (hi - v); }
                if (lpMix > 0.0f || prm.lpfOn) { float lo, hi; l.lp.process (v, lo, hi); v += lpMix * (lo - v); }

                const float recTarget = recordGainFor (l.group, prm);
                l.recSm = recTarget + recCoef * (l.recSm - recTarget);
                const float in = l.group == 0 ? gL : (l.group == 1 ? gR : inputMid);
                float w = (l.recSm * in * injScale + v * fbGain) * gComp;
                if (type == AlgoType::Reverb)
                {
                    auto& ap = allpasses[(size_t) i];
                    w = ap[1].process (ap[0].process (w));
                }
                if (std::fabs (w) > 0.5f) limiter = true;
                w = softLimit (w);
                blockPeak = std::max (blockPeak, std::fabs (w));
                w *= (1.0f - eraseRamp);

                const double inc = (double) (speed * dir);
                advanceAndWrite (l, w, inc, writeEnabled);
            }

            // ---------------- LFO phases
            for (int i = 0; i < numLines; ++i)
            {
                lfoPhase[(size_t) i] += lfoInc[(size_t) i];
                if (lfoPhase[(size_t) i] > kTwoPi) lfoPhase[(size_t) i] -= kTwoPi;
            }

            // ---------------- output
            const float dryG = std::min (1.0f, 2.0f * (1.0f - mix));
            const float wetG = std::min (1.0f, 2.0f * mix) * tGain
                               * ((type == AlgoType::TwoDelays || type == AlgoType::FourDelays) ? (1.0f - eraseRamp) : 1.0f);
            if (splitWet)
            {
                outL[s] = dryL * dryG * outG;
                outR[s] = dryR * dryG * outG;
                ext->wetL[s] = wetL;
                ext->wetR[s] = wetR;
                ext->wetGain[s] = wetG * outG;
            }
            else
            {
                outL[s] = (dryL * dryG + wetL * wetG) * outG;
                outR[s] = (dryR * dryG + wetR * wetG) * outG;
            }
        }

        meterPeak = blockPeak;
        inPeak = blockIn;
        limiterFlag = limiter;
        inClipFlag = clip;
    }

private:
    enum class TState { Idle, FadeOut, FadeIn, FadeOutErase, Clearing };

    //--------------------------------------------------------------------------
    void configure (int algo, bool fresh)
    {
        const AlgoType type = algoType (algo);
        const int v = algoVariant (algo);
        const int oldN = numLines;
        // old line boundaries become interior splices of the new layout: remember them
        int oldEdges[2 * kMaxLines] {};
        int nOldEdges = 0;
        if (! fresh)
            for (int i = 0; i < oldN; ++i)
            {
                oldEdges[nOldEdges++] = lines[(size_t) i].base;
                oldEdges[nOldEdges++] = lines[(size_t) i].base + lines[(size_t) i].len;
            }
        double oldFrac[kMaxLines] {};
        for (int i = 0; i < kMaxLines; ++i)
            oldFrac[i] = fresh || i >= oldN ? 0.0 : lines[(size_t) i].p / (double) lines[(size_t) i].len;

        std::array<double, kMaxLines> secs {};
        std::array<int, kMaxLines> groups {};
        std::array<float, kMaxLines> pans {};
        int nl = 2;

        switch (type)
        {
            case AlgoType::TwoDelays:
            {
                const double T[3] = { 2.5, 9.5, 22.0 };
                nl = 2;
                secs = { T[v], T[v] * 0.973 };
                groups = { 0, 1 };
                pans = { -1.0f, 1.0f };
                wetNorm = 1.0f; injScale = 1.0f;
                break;
            }
            case AlgoType::FourDelays:
            {
                const double T[3] = { 2.5, 8.5, 11.5 };
                nl = 4;
                secs = { T[v], T[v] * 0.966, T[v] * 0.937, T[v] * 0.904 };
                groups = { 0, 1, 0, 1 };
                pans = { -1.0f, 1.0f, -0.5f, 0.5f };
                wetNorm = 0.7071f; injScale = 1.0f;
                break;
            }
            case AlgoType::Reverb:
            {
                const double T[3] = { 1.1, 2.6, 5.3 };
                const double r[8] = { 1.000, 0.873, 0.781, 0.692, 0.611, 0.547, 0.479, 0.421 };
                nl = 8;
                for (int i = 0; i < 8; ++i) secs[(size_t) i] = T[v] * r[i];
                groups = { 0, 1, 0, 1, 0, 1, 0, 1 };
                pans = { -1.0f, 1.0f, -0.6f, 0.6f, -0.3f, 0.3f, -0.85f, 0.85f };
                wetNorm = 0.4f; injScale = 0.5f;
                const double apBase[8] = { 0.0071, 0.0113, 0.0157, 0.0193, 0.0239, 0.0283, 0.0331, 0.0379 };
                const double apMul[3] = { 1.0, 1.7, 2.9 };
                for (int i = 0; i < 8; ++i)
                {
                    allpasses[(size_t) i][0].setDelay (nextPrime ((int64_t) (apBase[i] * apMul[v] * sr)));
                    allpasses[(size_t) i][1].setDelay (nextPrime ((int64_t) (apBase[7 - i] * apMul[v] * 1.618 * sr)));
                    allpasses[(size_t) i][0].reset();
                    allpasses[(size_t) i][1].reset();
                }
                break;
            }
            case AlgoType::Granular:
            {
                const double T[3] = { 1.6, 4.2, 9.0 };
                const float gs[3] = { 0.045f, 0.14f, 0.38f };
                nl = v == 2 ? 3 : 2;
                secs = { T[v], T[v] * 0.971, T[v] * 1.27 };
                groups = { 0, 1, 2 };
                pans = { -0.75f, 0.75f, 0.0f };
                wetNorm = 1.0f; injScale = 1.0f;
                grainSec = gs[v];
                for (auto& gl : grains) for (auto& g : gl) g.active = false;
                for (auto& c : grainCountdown) c = 0.0f;
                break;
            }
        }

        // lay the lines out consecutively in the pool, lengths = distinct primes
        int base = 0;
        for (int i = 0; i < nl; ++i)
        {
            auto& l = lines[(size_t) i];
            int len = nextPrime ((int64_t) std::llround (secs[(size_t) i] * sr));
            len = std::min (len, poolSize - base - 8);
            l.base = base;
            l.len = std::max (16, len);
            base += l.len;
            l.group = groups[(size_t) i];
            l.panBase = pans[(size_t) i];
            // integer head position: at speed 1 the read is then exact (no interpolation
            // filtering accumulating on every pass)
            l.p = std::floor (oldFrac[i] * (double) l.len);
            if (l.p >= l.len) l.p = 0.0;
            l.acc = 0.0f; l.cnt = 0;
            l.curBin = -1;
        }
        numLines = nl;
        {
            int cnt[3] = { 0, 0, 0 };
            for (int i = 0; i < nl; ++i) ++cnt[std::clamp (lines[(size_t) i].group, 0, 2)];
            // a return channel feeds every line of its group: split it so the loop gain stays <= 1
            for (int g = 0; g < 3; ++g) groupNorm[(size_t) g] = cnt[g] > 0 ? 1.0f / (float) cnt[g] : 1.0f;
        }

        // glue points: short fades at the new line boundaries and at the old ones that are
        // now inside a line (masks discontinuities, like the small gaps SOMA describes in its FAQ)
        if (! fresh)
        {
            const int G = std::max (8, (int) (0.003f * sr));
            for (int k = 0; k < nOldEdges; ++k)
            {
                const int e = oldEdges[k];
                for (int j = 0; j < G; ++j)
                {
                    const float w = (float) j / (float) G;
                    if (e - 1 - j >= 0 && e - 1 - j < poolSize) pool[(size_t) (e - 1 - j)] *= w;
                    if (e + j >= 0 && e + j < poolSize) pool[(size_t) (e + j)] *= w;
                }
            }
            for (int i = 0; i < nl; ++i)
            {
                auto& l = lines[(size_t) i];
                const int g = std::min (G, l.len / 4);
                for (int k = 0; k < g; ++k)
                {
                    const float w = (float) k / (float) g;
                    pool[(size_t) (l.base + k)] *= w;
                    pool[(size_t) (l.base + l.len - 1 - k)] *= w;
                }
            }
        }
        for (int i = 0; i < nl; ++i) rebuildEnvelope (lines[(size_t) i]);
    }

    void rebuildEnvelope (Line& l)
    {
        for (int b = 0; b < kEnvBins; ++b)
        {
            const int s0 = (int) ((int64_t) b * l.len / kEnvBins);
            const int s1 = (int) ((int64_t) (b + 1) * l.len / kEnvBins);
            float pk = 0.0f;
            for (int k = s0; k < s1; k += 4) pk = std::max (pk, std::fabs (pool[(size_t) (l.base + k)]));
            l.env[b] = pk;
        }
    }

    //--------------------------------------------------------------------------
    inline float cell (const Line& l, int c) const
    {
        c %= l.len;
        if (c < 0) c += l.len;
        return pool[(size_t) (l.base + c)];
    }

    inline float readAt (const Line& l, double pos) const
    {
        const double fl = std::floor (pos);
        const int c = (int) fl;
        const float t = (float) (pos - fl);
        return hermite (cell (l, c - 1), cell (l, c), cell (l, c + 1), cell (l, c + 2), t);
    }

    inline void advanceAndWrite (Line& l, float w, double inc, bool writeEnabled)
    {
        l.acc += w;
        l.cnt += 1;
        const double pOld = l.p;
        double pNew = pOld + inc;
        const int cOld = (int) std::floor (pOld);
        const int cNew = (int) std::floor (pNew);
        if (cNew != cOld)
        {
            const float v = l.acc / (float) l.cnt;
            const int step = cNew > cOld ? 1 : -1;
            for (int c = cOld; c != cNew; c += step)
            {
                int cc = c % l.len;
                if (cc < 0) cc += l.len;
                if (writeEnabled)
                {
                    pool[(size_t) (l.base + cc)] = v;
                    const int bin = (int) ((int64_t) cc * kEnvBins / l.len);
                    if (bin != l.curBin) { l.curBin = bin; l.env[bin] = 0.0f; }
                    l.env[bin] = std::max (l.env[bin], std::fabs (v));
                }
            }
            l.acc = 0.0f;
            l.cnt = 0;
        }
        if (pNew >= (double) l.len) pNew -= (double) l.len;
        else if (pNew < 0.0) pNew += (double) l.len;
        l.p = pNew;
    }

    //--------------------------------------------------------------------------
    inline float granularRead (int i, float dir, float& wetL, float& wetR)
    {
        auto& l = lines[(size_t) i];
        auto& gs = grains[(size_t) i];
        const float gLen = grainSec * sr;

        grainCountdown[(size_t) i] -= 1.0f;
        if (grainCountdown[(size_t) i] <= 0.0f)
        {
            for (auto& g : gs)
            {
                if (g.active) continue;
                g.active = true;
                g.age = 0.0f;
                g.len = gLen * (0.75f + 0.5f * rng.uni());
                const float dc = (l.group == 2 ? 0.5f : 0.6f) * (float) l.len;
                const float mod = 0.35f * grainDrift * std::sin ((float) lfoPhase[(size_t) i]) + 0.45f * grainBlur * rng.bi();
                g.delay = std::clamp (dc * (1.0f + mod), 64.0f, (float) l.len - 64.0f);
                const float pan = std::clamp (l.panBase + 0.45f * rng.bi(), -1.0f, 1.0f);
                const float a = (pan + 1.0f) * 0.25f * kPi;
                g.gl = std::cos (a);
                g.gr = std::sin (a);
                break;
            }
            grainCountdown[(size_t) i] = (gLen / 2.5f) * (0.5f + rng.uni());
        }

        float mono = 0.0f;
        for (auto& g : gs)
        {
            if (! g.active) continue;
            const float ph = g.age / g.len;
            const float win = 0.5f - 0.5f * std::cos (kTwoPi * ph);
            const double pos = l.p - (double) dir * (double) g.delay;
            const float x = readAt (l, pos) * win;
            mono += x;
            wetL += x * g.gl * 0.8f;
            wetR += x * g.gr * 0.8f;
            g.age += 1.0f;
            if (g.age >= g.len) g.active = false;
        }
        l.out = mono * 0.8f;
        return l.out;
    }

    inline void mixLines (const float* in, float* out, AlgoType type, float blur) const
    {
        const int n = numLines;
        if (type == AlgoType::Reverb)
        {
            float sum = 0.0f;
            for (int i = 0; i < n; ++i) sum += in[i];
            const float k = 2.0f / (float) n;
            for (int i = 0; i < n; ++i) out[i] = in[i] - k * sum;
            return;
        }
        for (int i = 0; i < n; ++i) out[i] = in[i];
        if (type == AlgoType::Granular || blur <= 1.0e-4f) return;

        const float th = blur * 0.25f * kPi;
        const float c = std::cos (th), s = std::sin (th);
        auto rot = [c, s] (float& a, float& b) { const float x = a, y = b; a = c * x - s * y; b = s * x + c * y; };
        if (n == 2) rot (out[0], out[1]);
        else if (n == 4)
        {
            rot (out[0], out[1]); rot (out[2], out[3]);
            rot (out[0], out[2]); rot (out[1], out[3]);
        }
    }

    inline float recordGainFor (int group, const Params& p) const
    {
        if (! p.recOn) return 0.0f;
        if (p.recTarget == 2 || group == 2) return 1.0f;
        return group == p.recTarget ? 1.0f : 0.0f;
    }

    inline float gate (int ch, float x, float thr)
    {
        auto& st = gateSt[(size_t) ch];
        const float ax = std::fabs (x);
        st.env = ax > st.env ? ax : ax + gateCoefRel * (st.env - ax);
        if (st.env > thr) { st.open = true; st.hold = holdSamples; }
        else if (st.env < thr * 0.5f)
        {
            if (st.hold > 0) --st.hold;
            else st.open = false;
        }
        st.gain = st.open ? std::min (1.0f, st.gain + gateAtt) : std::max (0.0f, st.gain - gateRel);
        return x * st.gain;
    }

    void controlUpdate (const Params& p, float drv, float drift, AlgoType type)
    {
        // Lorenz attractor (slow chaos)
        const double dt = 1.0 / (40.0 * (double) sr / kControlRate);
        const double dx = 10.0 * (lorenzY - lorenzX);
        const double dy = lorenzX * (28.0 - lorenzZ) - lorenzY;
        const double dz = lorenzX * lorenzY - (8.0 / 3.0) * lorenzZ;
        lorenzX += dx * dt; lorenzY += dy * dt; lorenzZ += dz * dt;
        const float cx = (float) std::clamp (lorenzX / 20.0, -1.0, 1.0);
        chaos += 0.02f * (cx - chaos);

        static constexpr double ratios[kMaxLines] = { 1.0, 1.41421356, 1.61803399, 1.73205081, 1.11803399, 1.25992105, 1.53208889, 1.32471796 };
        const double f0 = 0.021 * (1.0 + 0.5 * (double) chaos);
        for (int i = 0; i < kMaxLines; ++i)
            lfoInc[(size_t) i] = kTwoPi * f0 * ratios[i] / (double) sr;

        // pan per line
        const bool driftPans = (type == AlgoType::TwoDelays || type == AlgoType::FourDelays);
        for (int i = 0; i < numLines; ++i)
        {
            auto& l = lines[(size_t) i];
            float pan = l.panBase;
            if (driftPans) pan = std::clamp (l.panBase * (1.0f - drift) + drift * std::sin ((float) lfoPhase[(size_t) i]), -1.0f, 1.0f);
            const float a = (pan + 1.0f) * 0.25f * kPi;
            l.panL = std::cos (a);
            l.panR = std::sin (a);
        }
        grainDrift = drift;
        grainBlur = smBlur.z;

        // drive coefficients
        drvK = 1.0f + 7.0f * drv;
        drvB = 0.15f * drv;
        drvTB = std::tanh (drvB);
        const float ref = std::tanh (0.5f * drvK + drvB) - drvTB;
        drvNorm = 0.5f / std::max (1.0e-3f, ref);

        // filters: crossfade + log-smoothed cutoff
        const float ctrlSec = (float) kControlRate / sr;
        const float mixStep = ctrlSec / 0.02f;
        hpMix = p.hpfOn ? std::min (1.0f, hpMix + mixStep) : std::max (0.0f, hpMix - mixStep);
        lpMix = p.lpfOn ? std::min (1.0f, lpMix + mixStep) : std::max (0.0f, lpMix - mixStep);
        const float hpT = kHpfFreqs[std::clamp (p.hpfFreq, 0, 2)];
        const float lpT = kLpfFreqs[std::clamp (p.lpfFreq, 0, 2)];
        const float k = 1.0f - std::exp (-ctrlSec / 0.02f);
        hpFc = std::exp (std::log (hpFc) + k * (std::log (hpT) - std::log (hpFc)));
        lpFc = std::exp (std::log (lpFc) + k * (std::log (lpT) - std::log (lpFc)));
        for (int i = 0; i < numLines; ++i)
        {
            lines[(size_t) i].hp.set (hpFc, sr);
            lines[(size_t) i].lp.set (lpFc, sr);
        }
    }

    //--------------------------------------------------------------------------
    float sr = 48000.0f;
    std::vector<float> pool;
    int poolSize = 0;

    std::array<Line, kMaxLines> lines {};
    int numLines = 2;
    float wetNorm = 1.0f, injScale = 1.0f;
    std::array<float, 3> groupNorm { 1.0f, 1.0f, 1.0f };

    std::array<std::array<Allpass, 2>, kMaxLines> allpasses {};
    std::array<std::array<Grain, kMaxGrains>, kMaxLines> grains {};
    std::array<float, kMaxLines> grainCountdown {};
    float grainSec = 0.1f, grainDrift = 0.0f, grainBlur = 0.0f;

    Smoother smIn, smBlur, smDrift, smDrive, smSup, smFb, smMix, smOut;
    AdaaDrive drvL, drvR;
    float drvK = 1.0f, drvB = 0.0f, drvTB = 0.0f, drvNorm = 1.0f;

    struct GateState { float env = 0.0f, gain = 0.0f; bool open = false; int hold = 0; };
    std::array<GateState, 2> gateSt {};
    float gateCoefRel = 0.0f, gateAtt = 0.0f, gateRel = 0.0f;
    int holdSamples = 0;

    float supEnv = 0.0f, supAtt = 0.0f, supRel = 0.0f;
    float compPow = 0.0f, compRms = 0.0f, compAtt = 0.0f, compRel = 0.0f, compGainDb = 0.0f;

    float dcR = 0.999f;
    float hpMix = 0.0f, lpMix = 0.0f, hpFc = 60.0f, lpFc = 9000.0f;

    std::array<double, kMaxLines> lfoPhase {}, lfoInc {};
    double lorenzX = 1.0, lorenzY = 1.0, lorenzZ = 20.0;
    float chaos = 0.0f;
    Rng rng;

    int ctrlCounter = 0;
    int currentAlgo = 2, targetAlgo = 2;
    TState tState = TState::Idle;
    float tGain = 1.0f;
    float speed = 1.0f, prevSpeed = 1.0f;
    float eraseRamp = 0.0f;
    bool eraseAllRequested = false, eraseAllPending = false, clearing = false;
    int clearPos = 0;

    float meterPeak = 0.0f, inPeak = 0.0f;
    bool limiterFlag = false, inClipFlag = false;
};

} // namespace deriva
