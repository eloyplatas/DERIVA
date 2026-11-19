// Offline tests for the Beads wrapper: resampling stream stability, pitch, latency, CPU
#include "../Source/dsp/BeadsUnit.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>
#include <xmmintrin.h>

using deriva::BeadsUnit;
static int failures = 0;
#define CHECK(c, ...) do { std::printf (c ? "  ok:   " : "  FAIL: "); std::printf (__VA_ARGS__); std::printf ("\n"); if (!(c)) ++failures; } while (0)

static void streamTest (double sr, int maxBlock, bool randomBlocks, int quality)
{
    BeadsUnit u; u.prepare (sr, maxBlock);
    BeadsUnit::Params p; p.quality = quality; p.dryWet = 1.0f; p.density = 0.75f; p.reverb = 0.3f; p.size = 0.6f;
    std::mt19937 rng (5);
    std::vector<float> L (maxBlock), R (maxBlock);
    long t = 0; const long total = (long) (sr * 40.0); bool bad = false; double e = 0;
    auto t0 = std::chrono::steady_clock::now();
    while (t < total)
    {
        int n = randomBlocks ? 1 + (int) (rng() % (unsigned) maxBlock) : maxBlock;
        for (int i = 0; i < n; ++i) { float x = 0.3f * std::sin (2.0 * M_PI * 330.0 * (t + i) / sr); L[i] = R[i] = x; }
        u.process (L.data(), R.data(), n, p, nullptr, 0);
        for (int i = 0; i < n; ++i) { if (! std::isfinite (L[i])) bad = true; e += L[i] * L[i]; }
        t += n;
    }
    const double cpu = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count() / 40.0 * 100.0;
    CHECK (! bad && u.underflowCount() == 0 && e > 1.0,
           "%6.0f Hz, blocks %s%-4d, quality %d: underflows %d, energy %.1f, nan %d, CPU %.2f %%", sr, randomBlocks ? "1..": "", maxBlock, quality, u.underflowCount(), e, (int) bad, cpu);
}

static void pitchAndLatency (double sr)
{
    // DRY/WET = 0 -> Beads passes the dry signal: measures the stream latency and pitch integrity
    BeadsUnit u; u.prepare (sr, 256);
    BeadsUnit::Params p; p.dryWet = 0.0f; p.reverb = 0.0f; p.quality = 3;
    std::vector<float> L (256), R (256);
    long t = 0, firstOut = -1; const long imp = (long) (sr * 0.5);
    for (int b = 0; b < (int) (sr / 256); ++b)
    {
        for (int i = 0; i < 256; ++i) { L[i] = R[i] = (t + i == imp) ? 1.0f : 0.0f; }
        u.process (L.data(), R.data(), 256, p, nullptr, 0);
        for (int i = 0; i < 256; ++i) if (firstOut < 0 && std::fabs (L[i]) > 0.05f) firstOut = t + i;
        t += 256;
    }
    const double latMs = (firstOut - imp) / sr * 1000.0;
    // pitch: 1 kHz sine through dry path
    long zc = 0, cnt = 0; float prev = 0;
    for (int b = 0; b < (int) (sr * 2 / 256); ++b)
    {
        for (int i = 0; i < 256; ++i) { L[i] = R[i] = 0.4f * std::sin (2.0 * M_PI * 1000.0 * (t + i) / sr); }
        u.process (L.data(), R.data(), 256, p, nullptr, 0);
        if (b > 20) for (int i = 0; i < 256; ++i) { if (prev <= 0.1f && L[i] > 0.1f) ++zc; if (L[i] < -0.1f) prev = L[i]; else if (L[i] > 0.1f) prev = L[i]; ++cnt; }
        t += 256;
    }
    const double f = zc / (cnt / sr);
    CHECK (firstOut > 0 && latMs < 35.0, "%6.0f Hz: wet-path latency %.1f ms", sr, latMs);
    CHECK (std::fabs (f - 1000.0) < 3.0, "%6.0f Hz: 1 kHz stays at %.1f Hz through the resampling chain", sr, f);
}

static void grainPitch()
{
    // PITCH knob at +12 st (detune law): grains one octave up
    const float knob = 0.5f + 0.5f * (5.0f / 6.0f + 0.01f); // 5th interval step = 12 st
    const float st = BeadsUnit::pitchSemitones (knob);
    CHECK (std::fabs (st - 12.0f) < 0.01f, "detune law: knob %.3f -> %.2f semitones", knob, st);
    CHECK (std::fabs (BeadsUnit::pitchSemitones (0.5f)) < 1e-6f, "detune law: centre = 0 semitones");
    CHECK (std::fabs (BeadsUnit::attenuverterLaw (0.0f)) < 1e-6f && std::fabs (BeadsUnit::attenuverterLaw (1.0f) - 1.0f) < 0.01f, "attenuverter law: 0 -> 0, 1 -> %.3f", BeadsUnit::attenuverterLaw (1.0f));
}

static void clockTest()
{
    BeadsUnit u; u.prepare (44100.0, 512);
    BeadsUnit::Params p; p.clockPatched = true; p.seedLock = false; p.density = 0.5f; p.dryWet = 1.0f; p.reverb = 0.0f; p.size = 0.3f;  // unlocked, density 12h: grains only on clock taps
    std::vector<float> L (512), R (512);
    double eNo = 0, eClk = 0; long t = 0;
    for (int b = 0; b < 400; ++b)
    {
        for (int i = 0; i < 512; ++i) L[i] = R[i] = 0.3f * std::sin (0.05f * (t + i));
        int pulse = 100;
        const bool clocked = b >= 200 && b % 20 == 0;
        u.process (L.data(), R.data(), 512, p, clocked ? &pulse : nullptr, clocked ? 1 : 0);
        for (int i = 0; i < 512; ++i) (b < 200 ? eNo : eClk) += L[i] * L[i];
        t += 512;
    }
    CHECK (eClk > 10.0 * (eNo + 1e-9), "clock pulses trigger grains (energy without clock %.3g, with clock %.3g)", eNo, eClk);
}

int main()
{
    _mm_setcsr (_mm_getcsr() | 0x8040);
    std::printf ("== Streams\n");
    for (double sr : { 44100.0, 48000.0, 88200.0, 96000.0 })
    {
        streamTest (sr, 512, false, 3);
        streamTest (sr, 512, true, 3);
        streamTest (sr, 64, true, 0);
    }
    streamTest (44100.0, 2048, true, 2);
    streamTest (44100.0, 441, false, 1);
    std::printf ("== Latency and pitch\n");
    for (double sr : { 44100.0, 48000.0, 96000.0 }) pitchAndLatency (sr);
    std::printf ("== Laws and clock\n");
    grainPitch();
    clockTest();
    std::printf ("\n%s (%d failures)\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED", failures);
    return failures ? 1 : 0;
}
