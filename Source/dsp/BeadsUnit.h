// Deriva — wrapper around the Beads DSP (Mutable Instruments, Emilie Gillet, MIT).
// Beads always runs at 48 kHz in fixed 32-sample blocks; when the host runs at another
// rate the signal is converted with r8brain (Aleksey Vaneev, MIT). A small jitter buffer
// absorbs the resampler latency and its output bursts.
// This header is free of Beads / stmlib includes so it can be used next to JUCE.
#pragma once

#include <memory>

namespace deriva
{
class BeadsUnit
{
public:
    struct Params
    {
        float time = 0.4f, timeRnd = 0.0f;   // knobs 0..1, random -1..1
        float size = 0.5f, sizeRnd = 0.0f;
        float shape = 0.5f, shapeRnd = 0.0f;
        float pitch = 0.5f, pitchRnd = 0.0f; // pitch knob 0..1 (detune law, centre = 0 st)
        float density = 0.7f;                // 0.5 = no grains, < 0.5 random, > 0.5 regular
        float feedback = 0.0f, dryWet = 0.5f, reverb = 0.3f;
        bool freeze = false, mono = false;
        bool seed = false;        // SEED held (manual grains / tap)
        bool seedLock = true;     // latched SEED = continuous grains at DENSITY (module default)
        bool clockPatched = false; // like a cable in the SEED jack: grains follow clockPulses
                                   // (latched: DENSITY = divider / probability of the clock)
        int quality = 3;                     // 0 dirty tape, 1 clean tape, 2 vintage digital, 3 digital
    };

    BeadsUnit();
    ~BeadsUnit();

    void prepare (double hostSampleRate, int maxBlockSize);
    void resetStreams();      // clears the resampling FIFOs (keeps the Beads memory)

    // In-place stereo processing at the host rate. clockPulses: sample offsets in this
    // block where a clock / seed pulse starts (may be null).
    void process (float* left, float* right, int numSamples, const Params& p, const int* clockPulses, int numPulses);

    float lastOutputPeak() const { return outPeak; }
    int underflowCount() const { return underflows; }
    double hostRate() const { return rate; }

    static float pitchSemitones (float knob);       // Beads LAW_DETUNE
    static float attenuverterLaw (float bipolar);   // Beads LAW_ATTENUVERTER, input -1..1

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    double rate = 48000.0;
    float outPeak = 0.0f;
    int underflows = 0;
};
} // namespace deriva
