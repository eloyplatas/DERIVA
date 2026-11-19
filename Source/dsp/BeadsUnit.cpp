#include "BeadsUnit.h"

#include "beads/dsp/processor.h"
#include "CDSPResampler.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <new>
#include <vector>

namespace deriva
{
namespace
{
    constexpr int kChunk = 32;
    constexpr double kBeadsRate = 48000.0;

    struct Fifo
    {
        std::vector<float> d;
        size_t r = 0;
        size_t size() const { return d.size() - r; }
        void clear() { d.clear(); r = 0; }
        void push (float v) { d.push_back (v); }
        float pop() { return d[r++]; }
        void compact()
        {
            if (r > 8192) { d.erase (d.begin(), d.begin() + (std::ptrdiff_t) r); r = 0; }
        }
    };
}

// Beads relies on its state starting zeroed (static storage on the module), so the
// processor is built in zero-filled memory.
struct ZeroedProcessor
{
    void* mem = nullptr;
    beads::Processor* p = nullptr;
    ZeroedProcessor()
    {
        mem = std::calloc (1, sizeof (beads::Processor));
        p = new (mem) beads::Processor();
    }
    ~ZeroedProcessor() { p->~Processor(); std::free (mem); }
    ZeroedProcessor (const ZeroedProcessor&) = delete;
    ZeroedProcessor& operator= (const ZeroedProcessor&) = delete;
};

struct BeadsUnit::Impl
{
    ZeroedProcessor zp;
    beads::Processor& proc = *zp.p;
    std::vector<uint8_t> delayMemory;
    std::vector<uint16_t> reverbMemory;
    uint8_t* buffers[3] {};

    bool resample = false;
    std::unique_ptr<r8b::CDSPResampler> up[2], down[2];
    std::vector<double> tmp;

    Fifo in48[2], out48[2], outHost[2];
    std::vector<uint8_t> gate48;   // pending clock gate per 48 kHz input sample
    size_t gateRead = 0;
    stmlib::GateFlags previousGate = stmlib::GATE_FLAG_LOW;
    int gateHold = 0;

    bool primed = false;
    size_t primeTarget = 0;
    int quality = -1;
    bool mono = false;
    // fade used when the quality / mono setting restarts the Beads buffer
    float fade = 1.0f;
    int fadeDir = 0;          // -1 fading out, +1 fading in
    int wantQuality = -1;
    bool wantMono = false;
};

BeadsUnit::BeadsUnit() : impl (std::make_unique<Impl>())
{
    auto& m = *impl;
    m.delayMemory.assign (768 * 1024, 0);
    m.reverbMemory.assign (32768, 0);
    m.buffers[0] = m.delayMemory.data();
    m.buffers[1] = m.delayMemory.data() + 256 * 1024;
    m.buffers[2] = m.delayMemory.data() + 512 * 1024;
    m.proc.Init (m.buffers, m.reverbMemory.data());
}

BeadsUnit::~BeadsUnit() = default;

void BeadsUnit::prepare (double hostSampleRate, int maxBlockSize)
{
    auto& m = *impl;
    rate = hostSampleRate;
    m.resample = std::abs (hostSampleRate - kBeadsRate) > 0.5;
    const int maxIn = std::max (64, maxBlockSize);
    for (int ch = 0; ch < 2; ++ch)
    {
        if (m.resample)
        {
            // 6 % transition band, 120 dB stop band, linear phase: ~19 ms for both conversions
            // (the 24-bit preset would add ~73 ms)
            m.up[ch] = std::make_unique<r8b::CDSPResampler> (hostSampleRate, kBeadsRate, maxIn, 6.0, 120.0, r8b::fprLinearPhase);
            m.down[ch] = std::make_unique<r8b::CDSPResampler> (kBeadsRate, hostSampleRate,
                                                               (int) std::ceil (maxIn * kBeadsRate / hostSampleRate) + 2 * kChunk + 64,
                                                               6.0, 120.0, r8b::fprLinearPhase);
        }
        else
        {
            m.up[ch].reset();
            m.down[ch].reset();
        }
    }
    m.tmp.assign ((size_t) (maxIn * 4 + 4096), 0.0);
    // jitter margin: one host block plus the largest output burst of the converters
    m.primeTarget = (size_t) (m.resample ? 192 : kChunk);
    m.quality = -1;
    resetStreams();
}

void BeadsUnit::resetStreams()
{
    auto& m = *impl;
    for (int ch = 0; ch < 2; ++ch)
    {
        m.in48[ch].clear(); m.out48[ch].clear(); m.outHost[ch].clear();
        if (m.up[ch]) m.up[ch]->clear();
        if (m.down[ch]) m.down[ch]->clear();
    }
    m.gate48.clear();
    m.gateRead = 0;
    m.primed = false;
}

float BeadsUnit::pitchSemitones (float knob)
{
    float detune = 2.0f * std::clamp (knob, 0.0f, 1.0f) - 1.0f;
    const float sign = detune < 0.0f ? -1.0f : 1.0f;
    detune = detune * sign * 6.0f;
    int integral = std::min (6, (int) detune);
    float fractional = detune - (float) integral;
    static const float intervals[8] = { 0.0f, 1.0f, 3.0f, 4.0f, 7.0f, 12.0f, 24.0f, 24.0f };
    fractional = std::max ((fractional - 0.33333333f) * 1.5f, 0.0f);
    const float s = fractional * fractional * (3.0f - 2.0f * fractional);
    return (intervals[integral] + (intervals[integral + 1] - intervals[integral]) * s) * sign;
}

float BeadsUnit::attenuverterLaw (float bipolar)
{
    const float v = 0.5f * std::clamp (bipolar, -1.0f, 1.0f);
    return (2.0f / 0.47f) * v * std::max (std::fabs (v) - 0.03f, 0.0f);
}

void BeadsUnit::process (float* left, float* right, int n, const Params& p, const int* pulses, int numPulses)
{
    auto& m = *impl;
    float* io[2] = { left, right };

    // ---- configuration (quality / mono) — like the module, this restarts the buffer.
    //      A 5 ms fade-out / fade-in hides the restart.
    const int q = std::clamp (p.quality, 0, 3);
    if (m.quality < 0)
    {
        m.proc.Configure ((beads::Processor::Media) q, p.mono);
        m.quality = q; m.mono = p.mono; m.fade = 1.0f; m.fadeDir = 0;
    }
    else if (q != m.quality || p.mono != m.mono)
    {
        m.wantQuality = q; m.wantMono = p.mono;
        if (m.fadeDir == 0) m.fadeDir = -1;
    }

    // ---- host -> 48 kHz
    const size_t before = m.in48[0].size() + m.in48[0].r;
    int produced = 0;
    for (int ch = 0; ch < 2; ++ch)
    {
        if (m.resample)
        {
            for (int i = 0; i < n; ++i) m.tmp[(size_t) i] = io[ch][i];
            double* op = nullptr;
            const int k = m.up[ch]->process (m.tmp.data(), n, op);
            for (int i = 0; i < k; ++i) m.in48[ch].push ((float) op[i]);
            produced = ch == 0 ? k : std::min (produced, k);
        }
        else
        {
            for (int i = 0; i < n; ++i) m.in48[ch].push (io[ch][i]);
            produced = n;
        }
    }
    (void) before;
    // clock gates aligned to the new 48 kHz samples
    {
        const size_t base = m.gate48.size();
        m.gate48.resize (base + (size_t) produced, 0);
        for (int k = 0; k < numPulses && pulses != nullptr && n > 0; ++k)
        {
            const size_t pos = base + (size_t) ((double) pulses[k] * (double) produced / (double) n);
            for (size_t j = pos; j < std::min (m.gate48.size(), pos + 240); ++j) m.gate48[j] = 1; // 5 ms gate
        }
    }

    // ---- Beads parameters
    beads::Parameters bp;
    auto rp = [] (float base, float rnd) { beads::RandomizedParameter r; r.base = base; r.attenuverter = attenuverterLaw (rnd); r.cv = 0.0f; r.has_cv = false; return r; };
    bp.time = rp (p.time, p.timeRnd);
    bp.size = rp (p.size, p.sizeRnd);
    bp.shape = rp (p.shape, p.shapeRnd);
    bp.pitch = rp (pitchSemitones (p.pitch), p.pitchRnd);
    bp.monophonic_mode = false;
    beads::ModulatedParameter density { std::clamp (p.density, 0.0f, 1.0f), 0.0f };
    beads::Processor::MixParameters mix;
    mix.feedback = std::clamp (p.feedback * 1.005f, 0.0f, 1.0f);
    mix.dry_wet = std::clamp (p.dryWet * 1.005f, 0.0f, 1.0f);
    mix.reverb_amount = std::clamp (p.reverb * 1.005f, 0.0f, 1.0f);
    mix.output_gate = false;
    const beads::SeedButtonState seed = p.seedLock ? beads::SEED_BUTTON_STATE_LOCKED
                                      : (p.seed ? beads::SEED_BUTTON_STATE_ON : beads::SEED_BUTTON_STATE_OFF);

    // ---- run Beads in 32-sample chunks
    beads::FloatFrame in[kChunk], out[kChunk];
    stmlib::GateFlags gates[kChunk];
    bool freeze[kChunk];
    while (m.in48[0].size() >= (size_t) kChunk && m.in48[1].size() >= (size_t) kChunk)
    {
        for (int i = 0; i < kChunk; ++i)
        {
            in[i].l = m.in48[0].pop();
            in[i].r = m.in48[1].pop();
            const bool g = m.gateRead < m.gate48.size() ? m.gate48[m.gateRead] != 0 : false;
            ++m.gateRead;
            m.previousGate = stmlib::ExtractGateFlags (m.previousGate, g);
            gates[i] = m.previousGate;
            freeze[i] = p.freeze;
        }
        m.proc.Process (seed, p.clockPatched ? gates : nullptr, freeze, density, bp, mix, in, out, kChunk);
        const float fStep = 1.0f / 240.0f; // 5 ms at 48 kHz
        for (int i = 0; i < kChunk; ++i)
        {
            if (m.fadeDir < 0) m.fade = std::max (0.0f, m.fade - fStep);
            else if (m.fadeDir > 0) m.fade = std::min (1.0f, m.fade + fStep);
            m.out48[0].push (out[i].l * m.fade);
            m.out48[1].push (out[i].r * m.fade);
        }
        if (m.fadeDir < 0 && m.fade <= 0.0f)
        {
            m.proc.Configure ((beads::Processor::Media) m.wantQuality, m.wantMono);
            m.quality = m.wantQuality; m.mono = m.wantMono;
            m.fadeDir = 1;
        }
        else if (m.fadeDir > 0 && m.fade >= 1.0f)
            m.fadeDir = 0;
    }
    if (m.gateRead > 8192) { m.gate48.erase (m.gate48.begin(), m.gate48.begin() + (std::ptrdiff_t) m.gateRead); m.gateRead = 0; }

    // ---- 48 kHz -> host
    for (int ch = 0; ch < 2; ++ch)
    {
        auto& o48 = m.out48[ch];
        if (m.resample)
        {
            const size_t k = o48.size();
            if (k > 0)
            {
                if (m.tmp.size() < k) m.tmp.resize (k);
                for (size_t i = 0; i < k; ++i) m.tmp[i] = o48.pop();
                double* op = nullptr;
                const int got = m.down[ch]->process (m.tmp.data(), (int) k, op);
                for (int i = 0; i < got; ++i) m.outHost[ch].push ((float) op[i]);
            }
        }
        else
        {
            while (o48.size() > 0) m.outHost[ch].push (o48.pop());
        }
        o48.compact();
        m.in48[ch].compact();
    }

    // ---- jitter buffer → output
    if (! m.primed && m.outHost[0].size() >= m.primeTarget + (size_t) n && m.outHost[1].size() >= m.primeTarget + (size_t) n)
        m.primed = true;

    float peak = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
    {
        auto& f = m.outHost[ch];
        for (int i = 0; i < n; ++i)
        {
            float v = 0.0f;
            if (m.primed)
            {
                if (f.size() > 0) v = f.pop();
                else { ++underflows; m.primed = false; }
            }
            io[ch][i] = v;
            peak = std::max (peak, std::fabs (v));
        }
        f.compact();
    }
    outPeak = peak;
}

} // namespace deriva
