// Offline tests for the Deriva engine (no JUCE needed)
#include "../Source/dsp/Engine.h"
#include <chrono>
#include <cstdio>
#include <random>
#include <xmmintrin.h>

using namespace deriva;

static constexpr double SR = 48000.0;
static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; std::printf ("  FAIL: "); std::printf (__VA_ARGS__); std::printf ("\n"); } else { std::printf ("  ok:   "); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

struct Runner
{
    Engine e;
    Engine::Params p;
    std::vector<float> iL, iR, oL, oR;
    static constexpr int B = 256;
    Runner() { e.prepare (SR); iL.resize (B); iR.resize (B); oL.resize (B); oR.resize (B); }

    // gen(n) -> sample ; cb(oL,oR,n)
    template <typename Gen, typename Cb>
    void run (double seconds, Gen gen, Cb cb)
    {
        long total = (long) (seconds * SR);
        for (long done = 0; done < total; done += B)
        {
            int n = (int) std::min<long> (B, total - done);
            for (int i = 0; i < n; ++i) { float x = gen (done + i); iL[i] = x; iR[i] = x; }
            e.process (iL.data(), iR.data(), oL.data(), oR.data(), n, p);
            cb (oL.data(), oR.data(), n);
        }
    }
};

static float rmsDb (double sumSq, long n) { return 10.0f * (float) std::log10 (sumSq / std::max (1L, n) + 1e-20); }

static void testHold (int algo, float blur)
{
    Runner r;
    r.p.algo = algo; r.p.fb = 1.0f; r.p.mix = 1.0f; r.p.drift = 0.0f; r.p.blur = blur;
    std::mt19937 rng (1);
    std::normal_distribution<float> nd (0.0f, 0.1f);
    // warm-up transition (fresh engine starts on algo 2)
    r.run (0.2, [] (long) { return 0.0f; }, [] (float*, float*, int) {});
    // fill the whole loop with noise (overwrite), then hold with FB = 1.0 and RECORD off
    r.p.fb = 0.0f;
    r.run (13.0, [&] (long) { return nd (rng); }, [] (float*, float*, int) {});
    r.p.fb = 1.0f; r.p.recOn = false;
    // Loop energy = energy stored in the memory pool (the output mix of 4 panned lines
    // legitimately changes with BLUR because it correlates the lines).
    std::vector<float> win;
    bool bad = false;
    auto poolEnergyDb = [&] {
        double e = 0; for (float v : r.e.memory()) e += (double) v * v;
        return 10.0f * (float) std::log10 (e + 1e-20);
    };
    for (int k = 0; k < 6; ++k)
    {
        r.run (25.0, [] (long) { return 0.0f; }, [&] (float* L, float* R, int n) {
            for (int i = 0; i < n; ++i) if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) bad = true;
        });
        win.push_back (poolEnergyDb());
    }
    float mn = 1e9f, mx = -1e9f;
    for (size_t i = 0; i < win.size(); ++i) { mn = std::min (mn, win[i]); mx = std::max (mx, win[i]); }
    CHECK (! bad, "algo %d blur %.1f: no NaN/Inf", algo, blur);
    CHECK (mx - mn < 0.5f, "algo %d blur %.1f: FB=1.0 holds level over 150 s (memory energy spread %.3f dB)", algo, blur, mx - mn);
}

static void testRunaway (int algo)
{
    Runner r;
    r.p.algo = algo; r.p.fb = 1.15f; r.p.mix = 1.0f; r.p.inGain = 2.0f;
    std::mt19937 rng (2);
    std::normal_distribution<float> nd (0.0f, 0.5f);
    r.run (0.3, [] (long) { return 0.0f; }, [] (float*, float*, int) {});
    float peak = 0; bool bad = false; double dc = 0; long cnt = 0;
    r.run (60.0, [&] (long i) { return i < (long) (3 * SR) ? nd (rng) : 0.0f; }, [&] (float* L, float* R, int n) {
        for (int i = 0; i < n; ++i)
        {
            if (! std::isfinite (L[i])) bad = true;
            peak = std::max (peak, std::max (std::fabs (L[i]), std::fabs (R[i])));
            dc += L[i]; ++cnt;
        }
    });
    CHECK (! bad && peak <= 1.001f, "algo %2d: FB=1.15 bounded by limiter + bus safety (peak %.2f)", algo, peak);
    CHECK (std::fabs (dc / cnt) < 0.01, "algo %2d: no DC build-up (mean %.5f)", algo, dc / cnt);
}

static void testAlgoSwitchClicks()
{
    // Steps measured inside +-60 ms of each algorithm switch must not exceed the
    // largest step the memory content itself produces away from switches.
    Runner r;
    r.p.algo = 3; r.p.fb = 0.0f; r.p.mix = 1.0f; r.p.drift = 0.0f;
    const float w = (float) (2.0 * M_PI * 220.0 / SR);
    r.run (23.0, [&] (long i) { return 0.3f * std::sin (w * i); }, [] (float*, float*, int) {});
    r.p.recOn = false; r.p.fb = 1.0f;
    float inWin = 0, outWin = 0, prevL = 0, prevR = 0; bool bad = false;
    const long winS = (long) (0.06 * SR);
    for (int k = 0; k < 100; ++k)
    {
        r.p.algo = 1 + (k * 5) % 12;
        long t = 0;
        r.run (0.6, [] (long) { return 0.0f; }, [&] (float* L, float* R, int n) {
            for (int i = 0; i < n; ++i, ++t)
            {
                if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) bad = true;
                const float st = std::max (std::fabs (L[i] - prevL), std::fabs (R[i] - prevR));
                prevL = L[i]; prevR = R[i];
                if (t < winS) inWin = std::max (inWin, st); else outWin = std::max (outWin, st);
            }
        });
    }
    CHECK (! bad, "100 algorithm switches: no NaN");
    CHECK (inWin <= outWin * 1.05f, "100 algorithm switches without clicks (max step at switch %.4f, elsewhere %.4f)", inWin, outWin);
}

static void testHalfSpeed()
{
    Runner r;
    r.p.algo = 1; r.p.fb = 1.0f; r.p.mix = 1.0f; r.p.drift = 0.0f;
    r.run (0.2, [] (long) { return 0.0f; }, [] (float*, float*, int) {});
    const float w = (float) (2.0 * M_PI * 1000.0 / SR);
    r.run (3.0, [&] (long i) { return 0.3f * std::sin (w * i); }, [] (float*, float*, int) {});
    r.p.recOn = false;
    r.p.halfSpeed = true;
    r.run (1.0, [] (long) { return 0.0f; }, [] (float*, float*, int) {});
    long zc = 0, n0 = 0; float prev = 0;
    r.run (2.0, [] (long) { return 0.0f; }, [&] (float* L, float*, int n) { for (int i = 0; i < n; ++i) { if ((prev <= 0 && L[i] > 0)) ++zc; prev = L[i]; ++n0; } });
    const double f = zc / (n0 / SR);
    CHECK (std::fabs (f - 500.0) < 15.0, "half speed: 1000 Hz plays back at %.1f Hz (expected 500)", f);
    r.p.halfSpeed = false;
    r.run (1.0, [] (long) { return 0.0f; }, [] (float*, float*, int) {});
    zc = 0; n0 = 0;
    r.run (2.0, [] (long) { return 0.0f; }, [&] (float* L, float*, int n) { for (int i = 0; i < n; ++i) { if ((prev <= 0 && L[i] > 0)) ++zc; prev = L[i]; ++n0; } });
    const double f2 = zc / (n0 / SR);
    CHECK (std::fabs (f2 - 1000.0) < 20.0, "back to normal speed: %.1f Hz (expected 1000)", f2);
}

static void testAllAlgos()
{
    for (int a = 1; a <= 12; ++a)
    {
        Runner r;
        r.p.algo = a; r.p.fb = 0.97f; r.p.mix = 1.0f; r.p.blur = 0.5f; r.p.drift = 0.5f;
        std::mt19937 rng (a);
        std::normal_distribution<float> nd (0.0f, 0.1f);
        double acc = 0; long cnt = 0; bool bad = false;
        auto t0 = std::chrono::steady_clock::now();
        r.run (30.0, [&] (long i) { return i < (long) (2 * SR) ? nd (rng) : 0.0f; }, [&] (float* L, float* R, int n) {
            for (int i = 0; i < n; ++i) { if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) bad = true; acc += L[i] * L[i]; ++cnt; }
        });
        auto t1 = std::chrono::steady_clock::now();
        const double cpu = std::chrono::duration<double> (t1 - t0).count() / 30.0 * 100.0;
        CHECK (! bad && acc > 1e-3, "algo %2d: output alive, %.1f dBFS RMS, CPU %.2f %% of one core", a, rmsDb (acc, cnt), cpu);
    }
}

static void testEraseAll()
{
    Runner r;
    r.p.algo = 3; r.p.fb = 1.0f; r.p.mix = 1.0f;
    r.run (2.0, [] (long i) { return 0.3f * std::sin (0.05f * i); }, [] (float*, float*, int) {});
    r.p.recOn = false;
    r.e.requestEraseAll();
    r.run (0.5, [] (long) { return 0.0f; }, [] (float*, float*, int) {});
    float pk = 0;
    r.run (25.0, [] (long) { return 0.0f; }, [&] (float* L, float* R, int n) { for (int i = 0; i < n; ++i) pk = std::max (pk, std::fabs (L[i]) + std::fabs (R[i])); });
    CHECK (pk < 1e-6f, "ERASE ALL leaves the memory silent (peak %.2e)", pk);
}

static void testSampleRates()
{
    for (double sr : { 44100.0, 96000.0 })
    {
        Engine e; e.prepare (sr);
        Engine::Params p; p.algo = 2;
        std::vector<float> a (512, 0.1f), L (512), R (512);
        for (int k = 0; k < 400; ++k) e.process (a.data(), a.data(), L.data(), R.data(), 512, p);
        const float lenSec = e.lineLengthSeconds (0);
        CHECK (std::fabs (lenSec - 9.5f) < 0.01f, "sr %.0f: line A = %.3f s (expected 9.5)", sr, lenSec);
    }
}

static void testSplitWet()
{
    // dry + wetGain * wet (split mode) must equal the normal output
    Engine a, b; a.prepare (SR); b.prepare (SR);
    Engine::Params p; p.algo = 5; p.mix = 0.4f; p.drift = 0.5f;
    const int B = 256;
    std::vector<float> iL (B), iR (B), oL (B), oR (B), sL (B), sR (B), wL (B), wR (B), g (B);
    Engine::ExternalIO io; io.wetL = wL.data(); io.wetR = wR.data(); io.wetGain = g.data();
    float maxErr = 0;
    for (int k = 0; k < 2000; ++k)
    {
        for (int i = 0; i < B; ++i) iL[i] = iR[i] = 0.2f * std::sin (0.01f * (float) (k * B + i));
        a.process (iL.data(), iR.data(), oL.data(), oR.data(), B, p);
        b.process (iL.data(), iR.data(), sL.data(), sR.data(), B, p, &io);
        for (int i = 0; i < B; ++i) maxErr = std::max (maxErr, std::fabs (oL[i] - (sL[i] + g[i] * wL[i])));
    }
    CHECK (maxErr < 1e-5f, "split-wet output identical to normal output (max error %.2e)", maxErr);
}

int main()
{
    _mm_setcsr (_mm_getcsr() | 0x8040); // FTZ + DAZ, as juce::ScopedNoDenormals does in the plugin
    std::printf ("== FB = 1.0 holds level\n");
    testHold (2, 0.0f); testHold (2, 1.0f); testHold (5, 0.5f); testHold (5, 1.0f);
    std::printf ("== Runaway protection\n");
    for (int a : { 1, 4, 8, 11 }) testRunaway (a);
    std::printf ("== Transitions\n");
    testAlgoSwitchClicks();
    std::printf ("== Half speed\n");
    testHalfSpeed();
    std::printf ("== All algorithms + CPU\n");
    testAllAlgos();
    std::printf ("== Erase all\n");
    testEraseAll();
    std::printf ("== Split wet (Beads hook)\n");
    testSplitWet();
    std::printf ("== Sample rates\n");
    testSampleRates();
    std::printf ("\n%s (%d failures)\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED", failures);
    return failures ? 1 : 0;
}
