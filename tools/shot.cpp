// Renders the editor to a PNG after feeding the engine with test audio (development tool)
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    if (argc > 1 && String (argv[1]) == "--beads-stress")
    {
        // whole-plugin stress test of the Beads stage at 44.1 kHz with random block sizes
        DerivaProcessor a;
        a.setRateAndBufferSizeDetails (44100.0, 1024);
        a.prepareToPlay (44100.0, 1024);
        auto set = [&] (const char* id, float plain) { auto* p = a.apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (plain)); };
        set (ParamIDs::fb, 1.0f); set (ParamIDs::mix, 0.7f);
        AudioBuffer<float> b (2, 1024); MidiBuffer m; Random rnd (11);
        float peak = 0; bool bad = false; double maxStep = 0; float prev = 0; long t = 0;
        const auto t0 = Time::getMillisecondCounterHiRes();
        const double seconds = 240.0;
        const int onlyEvent = argc > 2 ? atoi (argv[2]) : -1;
        int events = 0;
        while (t < (long) (seconds * 44100))
        {
            const int n = 1 + rnd.nextInt (1024);
            if (rnd.nextInt (60) == 0)
            {
                ++events;
                const int ev = onlyEvent >= 0 ? onlyEvent : rnd.nextInt (6);
                switch (ev)
                {
                    case 0: set (ParamIDs::beadsOn, rnd.nextBool() ? 1.0f : 0.0f); break;
                    case 1: set (ParamIDs::bPlace, (float) rnd.nextInt (2)); break;
                    case 2: set (ParamIDs::bQuality, (float) rnd.nextInt (4)); break;
                    case 3: set (ParamIDs::bSize, rnd.nextFloat()); set (ParamIDs::bPitch, rnd.nextFloat()); break;
                    case 4: set (ParamIDs::algo, (float) (1 + rnd.nextInt (12))); break;
                    default: set (ParamIDs::bDryWet, rnd.nextFloat()); set (ParamIDs::bFeedback, rnd.nextFloat()); set (ParamIDs::bFreeze, (float) rnd.nextInt (2)); break;
                }
            }
            b.setSize (2, n, false, false, true);
            for (int i = 0; i < n; ++i)
            {
                const float x = ((t + i) % 88200 < 30000) ? 0.4f * std::sin (0.04f * (float) (t + i)) : 0.0f;
                b.setSample (0, i, x); b.setSample (1, i, x);
            }
            a.processBlock (b, m);
            for (int i = 0; i < n; ++i)
            {
                const float v = b.getSample (0, i);
                if (! std::isfinite (v) || ! std::isfinite (b.getSample (1, i))) bad = true;
                peak = std::max (peak, std::abs (v));
                maxStep = std::max (maxStep, (double) std::abs (v - prev)); prev = v;
            }
            t += n;
        }
        const double cpu = (Time::getMillisecondCounterHiRes() - t0) / 1000.0 / seconds * 100.0;
        std::printf ("beads stress: %d random changes, NaN %s, peak %.2f, max step %.3f, CPU %.2f %% of one core -> %s\n",
                     events, bad ? "YES" : "no", peak, maxStep, cpu, (! bad && peak < 2.5f) ? "OK" : "FAIL");
        return (! bad && peak < 2.5f) ? 0 : 1;
    }
    if (argc > 1 && String (argv[1]) == "--state")
    {
        // save-memory round trip: fill memory, save state, load into a fresh instance
        DerivaProcessor a;
        a.setRateAndBufferSizeDetails (48000.0, 512);
        a.prepareToPlay (48000.0, 512);
        a.apvts.getParameter (ParamIDs::saveMemory)->setValueNotifyingHost (1.0f);
        a.apvts.getParameter (ParamIDs::algo)->setValueNotifyingHost (a.apvts.getParameter (ParamIDs::algo)->convertTo0to1 (3.0f));
        AudioBuffer<float> b (2, 512); MidiBuffer m;
        for (int k = 0; k < 48000 * 25 / 512; ++k)
        {
            for (int i = 0; i < 512; ++i) { const float x = 0.3f * std::sin (0.031f * (float) (k * 512 + i)); b.setSample (0, i, x); b.setSample (1, i, x); }
            a.processBlock (b, m);
        }
        MemoryBlock st;
        a.getStateInformation (st);
        DerivaProcessor c;
        c.setRateAndBufferSizeDetails (48000.0, 512);
        c.prepareToPlay (48000.0, 512);
        c.setStateInformation (st.getData(), (int) st.getSize());
        b.clear();
        c.processBlock (b, m);
        auto& ma = const_cast<deriva::Engine&> (a.getEngine()).memory();
        auto& mc = const_cast<deriva::Engine&> (c.getEngine()).memory();
        double err = 0, energy = 0;
        for (size_t i = 0; i < ma.size(); ++i) { err = std::max (err, (double) std::abs (ma[i] - mc[i])); energy += ma[i] * ma[i]; }
        std::printf ("state size %.2f MB, memory energy %.1f, max abs error after restore %.6f -> %s\n",
                     st.getSize() / 1048576.0, energy, err, (err < 1.0e-3 && energy > 1.0) ? "OK" : "FAIL");
        return (err < 1.0e-3 && energy > 1.0) ? 0 : 1;
    }
    const int algo = argc > 1 ? atoi (argv[1]) : 5;
    const String outPath = argc > 2 ? String (argv[2]) : String ("shot.png");
    const double seconds = argc > 3 ? atof (argv[3]) : 30.0;

    auto proc = std::make_unique<DerivaProcessor>();
    proc->setRateAndBufferSizeDetails (48000.0, 512);
    proc->prepareToPlay (48000.0, 512);
    auto set = [&] (const char* id, float plain) { auto* p = proc->apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (plain)); };
    set (ParamIDs::algo, (float) algo);
    set (ParamIDs::drift, 0.55f);
    set (ParamIDs::blur, 0.35f);
    set (ParamIDs::fb, 0.985f);
    set (ParamIDs::mix, 0.6f);
    set (ParamIDs::lpfOn, 1.0f);
    const bool withBeads = argc > 4 && String (argv[4]) == "beads";
    if (withBeads)
    {
        set (ParamIDs::beadsOn, 1.0f);
        set (ParamIDs::bTime, 0.35f); set (ParamIDs::bTimeRnd, 0.4f);
        set (ParamIDs::bSize, 0.45f); set (ParamIDs::bSizeRnd, -0.3f);
        set (ParamIDs::bPitch, 0.92f); set (ParamIDs::bPitchRnd, 0.15f);
        set (ParamIDs::bDensity, 0.72f); set (ParamIDs::bDryWet, 0.7f); set (ParamIDs::bReverb, 0.45f);
        set (ParamIDs::bQuality, 1.0f);
    }

    AudioBuffer<float> buf (2, 512);
    MidiBuffer midi;
    Random rnd (7);
    const double sr = 48000.0;
    long t = 0;
    double phase = 0, freq = 220, env = 0;
    const long total = (long) (seconds * sr);
    for (long done = 0; done < total; done += 512)
    {
        for (int i = 0; i < 512; ++i, ++t)
        {
            if (t % (long) (sr * 1.7) == 0) { freq = 110.0 * std::pow (2.0, rnd.nextInt (24) / 12.0); env = 0.6; }
            env *= 0.99993;
            phase += freq / sr;
            const float x = (float) (env * (std::sin (2 * M_PI * phase) * 0.7 + 0.3 * std::sin (6 * M_PI * phase)));
            buf.setSample (0, i, x);
            buf.setSample (1, i, x);
        }
        proc->processBlock (buf, midi);
    }

    std::unique_ptr<AudioProcessorEditor> ed (proc->createEditor());
    for (int k = 0; k < 20; ++k) static_cast<DerivaEditor*> (ed.get())->refreshNow();
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
    File f (File::getCurrentWorkingDirectory().getChildFile (outPath));
    f.deleteFile();
    FileOutputStream os (f);
    PNGImageFormat().writeImageToStream (img, os);
    std::printf ("wrote %s\n", f.getFullPathName().toRawUTF8());
    ed.reset();
    return 0;
}
