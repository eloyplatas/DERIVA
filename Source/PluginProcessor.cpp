#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

namespace
{
    String dotName (const char* a, const char* b) { return String (a) + String (CharPointer_UTF8 (" \xc2\xb7 ")) + String (b); }
    const StringArray algoNames { dotName ("2 DELAYS", "S"), dotName ("2 DELAYS", "M"), dotName ("2 DELAYS", "L"),
                                  dotName ("4 DELAYS", "S"), dotName ("4 DELAYS", "M"), dotName ("4 DELAYS", "L"),
                                  dotName ("REVERB", "HUGE"), dotName ("REVERB", "SUPER"), dotName ("REVERB", "INSANE"),
                                  dotName ("GRAIN", "S"), dotName ("GRAIN", "M"), dotName ("GRAIN", "L") };
    constexpr int kStateMagic = 0x44525631; // "DRV1"
}

AudioProcessorValueTreeState::ParameterLayout DerivaProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pct = [] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; };

    l.add (std::make_unique<AudioParameterInt> (ParameterID { ParamIDs::algo, 1 }, "Algorithm", 1, 12, 2,
        AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return algoNames[jlimit (0, 11, v - 1)]; })));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::in, 1 }, "In", NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::blur, 1 }, "Blur", NormalisableRange<float> (0.0f, 1.0f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::drift, 1 }, "Drift", NormalisableRange<float> (0.0f, 1.0f), 0.3f,
        AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::drive, 1 }, "Drive", NormalisableRange<float> (0.0f, 1.0f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::supcom, 1 }, "Sup|Com", NormalisableRange<float> (-1.0f, 1.0f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) {
            if (std::abs (v) <= 0.02f) return String ("OFF");
            return (v < 0 ? String ("SUP ") : String ("COM ")) + String (roundToInt (std::abs (v) * 100.0f)) + " %";
        })));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::fb, 1 }, "Feedback", NormalisableRange<float> (0.0f, 1.15f), 0.97f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 3); })));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::mix, 1 }, "Mix", NormalisableRange<float> (0.0f, 1.0f), 0.5f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) {
            return String (roundToInt ((1.0f - v) * 100.0f)) + " / " + String (roundToInt (v * 100.0f));
        })));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::out, 1 }, "Out", NormalisableRange<float> (-60.0f, 12.0f, 0.01f, 2.0f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction ([] (float v, int) {
            return v <= -59.9f ? String ("-inf") : String (v, 1) + " dB";
        })));

    const StringArray freqNames { "MIN", "MED", "MAX" };
    // switches are 2-state choices (they round-trip exactly through host state, unlike AudioParameterBool)
    const StringArray onOff { "OFF", "ON" };
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::hpfOn, 1 }, "HPF", onOff, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::hpfFreq, 1 }, "HPF Cutoff", freqNames, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::lpfOn, 1 }, "LPF", onOff, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::lpfFreq, 1 }, "LPF Cutoff", freqNames, 1));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::recOn, 1 }, "Record", onOff, 1));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::recTarget, 1 }, "Record Target", StringArray { "GROUP 1", "GROUP 2", "ALL" }, 2));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::reverse, 1 }, "Reverse", onOff, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::halfSpeed, 1 }, "Half Speed", onOff, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::erase, 1 }, "Erase", onOff, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::eraseAll, 1 }, "Erase All", onOff, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::inputMode, 1 }, "Input", StringArray { "STEREO", "MONO SUM" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::gate, 1 }, "Gate", StringArray { "NORMAL", "SENSITIVE" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::saveMemory, 1 }, "Save Memory", onOff, 0));

    // ---- Beads stage (DSP by Emilie Gillet, Mutable Instruments, MIT)
    auto bip = [] (float v, int) { return std::abs (v) < 0.005f ? String ("0") : (v > 0 ? String ("+") : String ("-")) + String (roundToInt (std::abs (v) * 100.0f)) + " %"; };
    auto unit = [] (const char* id, const char* name, float def, std::function<String (float, int)> txt)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 1.0f), def,
                                                      AudioParameterFloatAttributes().withStringFromValueFunction (txt));
    };
    auto rnd = [&] (const char* id, const char* name)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (-1.0f, 1.0f), 0.0f,
                                                      AudioParameterFloatAttributes().withStringFromValueFunction (bip));
    };
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::beadsOn, 1 }, "Beads", onOff, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::bPlace, 1 }, "Beads Place", StringArray { "WET", "LOOP" }, 0));
    l.add (unit (ParamIDs::bTime, "Beads Time", 0.4f, pct));
    l.add (rnd (ParamIDs::bTimeRnd, "Beads Time Random"));
    l.add (unit (ParamIDs::bSize, "Beads Size", 0.55f, [] (float v, int) {
        if (v > 0.99f) return String ("DELAY");
        return (v < 0.3333f ? String ("REV ") : String ("")) + String (roundToInt (v * 100.0f)) + " %"; }));
    l.add (rnd (ParamIDs::bSizeRnd, "Beads Size Random"));
    l.add (unit (ParamIDs::bShape, "Beads Shape", 0.5f, pct));
    l.add (rnd (ParamIDs::bShapeRnd, "Beads Shape Random"));
    l.add (unit (ParamIDs::bPitch, "Beads Pitch", 0.5f, [] (float v, int) {
        const float st = deriva::BeadsUnit::pitchSemitones (v);
        return (st > 0.005f ? String ("+") : String ("")) + String (st, std::abs (st - std::round (st)) < 0.01f ? 0 : 1) + " st"; }));
    l.add (rnd (ParamIDs::bPitchRnd, "Beads Pitch Random"));
    l.add (unit (ParamIDs::bDensity, "Beads Density", 0.7f, [] (float v, int) {
        if (v < 0.48f) return String ("RANDOM ") + String (roundToInt ((0.48f - v) / 0.48f * 100.0f)) + " %";
        if (v > 0.52f) return String ("REGULAR ") + String (roundToInt ((v - 0.52f) / 0.48f * 100.0f)) + " %";
        return String ("MANUAL"); }));
    l.add (unit (ParamIDs::bFeedback, "Beads Feedback", 0.0f, pct));
    l.add (unit (ParamIDs::bDryWet, "Beads Dry/Wet", 0.6f, [] (float v, int) {
        return String (roundToInt ((1.0f - v) * 100.0f)) + " / " + String (roundToInt (v * 100.0f)); }));
    l.add (unit (ParamIDs::bReverb, "Beads Reverb", 0.25f, pct));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::bFreeze, 1 }, "Beads Freeze", onOff, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::bSeed, 1 }, "Beads Seed", onOff, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::bSeedLock, 1 }, "Beads Seed Latch", onOff, 1));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::bQuality, 1 }, "Beads Quality",
                                                   StringArray { "DIRTY TAPE", "CLEAN TAPE", "VINTAGE", "DIGITAL" }, 3));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::bMono, 1 }, "Beads Mono", onOff, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::bSync, 1 }, "Beads Sync", onOff, 0));
    return l;
}

DerivaProcessor::DerivaProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DERIVA", createLayout())
{
    pAlgo = apvts.getRawParameterValue (ParamIDs::algo);
    pIn = apvts.getRawParameterValue (ParamIDs::in);
    pBlur = apvts.getRawParameterValue (ParamIDs::blur);
    pDrift = apvts.getRawParameterValue (ParamIDs::drift);
    pDrive = apvts.getRawParameterValue (ParamIDs::drive);
    pSup = apvts.getRawParameterValue (ParamIDs::supcom);
    pFb = apvts.getRawParameterValue (ParamIDs::fb);
    pMix = apvts.getRawParameterValue (ParamIDs::mix);
    pOut = apvts.getRawParameterValue (ParamIDs::out);
    pHpfOn = apvts.getRawParameterValue (ParamIDs::hpfOn);
    pHpfFreq = apvts.getRawParameterValue (ParamIDs::hpfFreq);
    pLpfOn = apvts.getRawParameterValue (ParamIDs::lpfOn);
    pLpfFreq = apvts.getRawParameterValue (ParamIDs::lpfFreq);
    pRecOn = apvts.getRawParameterValue (ParamIDs::recOn);
    pRecTarget = apvts.getRawParameterValue (ParamIDs::recTarget);
    pReverse = apvts.getRawParameterValue (ParamIDs::reverse);
    pHalf = apvts.getRawParameterValue (ParamIDs::halfSpeed);
    pErase = apvts.getRawParameterValue (ParamIDs::erase);
    pEraseAll = apvts.getRawParameterValue (ParamIDs::eraseAll);
    pInputMode = apvts.getRawParameterValue (ParamIDs::inputMode);
    pGate = apvts.getRawParameterValue (ParamIDs::gate);
    pSaveMem = apvts.getRawParameterValue (ParamIDs::saveMemory);
    pBeadsOn = apvts.getRawParameterValue (ParamIDs::beadsOn);
    pBPlace = apvts.getRawParameterValue (ParamIDs::bPlace);
    pBTime = apvts.getRawParameterValue (ParamIDs::bTime);
    pBTimeRnd = apvts.getRawParameterValue (ParamIDs::bTimeRnd);
    pBSize = apvts.getRawParameterValue (ParamIDs::bSize);
    pBSizeRnd = apvts.getRawParameterValue (ParamIDs::bSizeRnd);
    pBShape = apvts.getRawParameterValue (ParamIDs::bShape);
    pBShapeRnd = apvts.getRawParameterValue (ParamIDs::bShapeRnd);
    pBPitch = apvts.getRawParameterValue (ParamIDs::bPitch);
    pBPitchRnd = apvts.getRawParameterValue (ParamIDs::bPitchRnd);
    pBDensity = apvts.getRawParameterValue (ParamIDs::bDensity);
    pBFeedback = apvts.getRawParameterValue (ParamIDs::bFeedback);
    pBDryWet = apvts.getRawParameterValue (ParamIDs::bDryWet);
    pBReverb = apvts.getRawParameterValue (ParamIDs::bReverb);
    pBFreeze = apvts.getRawParameterValue (ParamIDs::bFreeze);
    pBSeed = apvts.getRawParameterValue (ParamIDs::bSeed);
    pBSeedLock = apvts.getRawParameterValue (ParamIDs::bSeedLock);
    pBQuality = apvts.getRawParameterValue (ParamIDs::bQuality);
    pBMono = apvts.getRawParameterValue (ParamIDs::bMono);
    pBSync = apvts.getRawParameterValue (ParamIDs::bSync);
}

bool DerivaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::stereo()) return false;
    const auto in = layouts.getMainInputChannelSet();
    return in == AudioChannelSet::stereo() || in == AudioChannelSet::mono();
}

void DerivaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Beads stage buffers (any host block is processed in chunks of at most `capacity`)
    capacity = jmax (512, samplesPerBlock);
    for (auto* v : { &wetL, &wetR, &wetGain, &bL, &bR, &retL, &retR })
        v->assign ((size_t) capacity, 0.0f);
    retFifoL.assign ((size_t) capacity, 0.0f);
    retFifoR.assign ((size_t) capacity, 0.0f);
    retFifoL.reserve ((size_t) capacity * 4);
    retFifoR.reserve ((size_t) capacity * 4);
    retRead = 0;
    pulseScratch.reserve (64);
    beads.prepare (sampleRate, capacity);
    beadsRunning = false;

    // Hosts call prepareToPlay on many occasions; only rebuild (and lose the memory)
    // when the sample rate really changes.
    if (! prepared.load() || sampleRate != preparedRate)
    {
        prepared.store (false);
        engine.prepare (sampleRate);
        preparedRate = sampleRate;
        prepared.store (true);
    }
}

void DerivaProcessor::setParam (const char* id, float plainValue)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

void DerivaProcessor::toggleParam (const char* id)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f);
}

void DerivaProcessor::handleMidi (const MidiBuffer& midi)
{
    // Footswitch notes (any channel): 60 HPF, 61 LPF, 62 ERASE (momentary), 63 REVERSE,
    // 64 RECORD, 65 ERASE ALL, 66 HALF SPEED. CC 20-27: IN BLUR DRIFT DRIVE SUP|COM FB MIX OUT; CC 28: algorithm.
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            switch (m.getNoteNumber())
            {
                case 60: toggleParam (ParamIDs::hpfOn); break;
                case 61: toggleParam (ParamIDs::lpfOn); break;
                case 62: setParam (ParamIDs::erase, 1.0f); break;
                case 63: toggleParam (ParamIDs::reverse); break;
                case 64: toggleParam (ParamIDs::recOn); break;
                case 65: eraseAllFromUi.store (true); break;
                case 66: toggleParam (ParamIDs::halfSpeed); break;
                case 67: toggleParam (ParamIDs::bFreeze); break;
                case 68: setParam (ParamIDs::bSeedLock, 0.0f); setParam (ParamIDs::bSeed, 1.0f); break;
                case 69: toggleParam (ParamIDs::beadsOn); break;
                default: break;
            }
        }
        else if (m.isNoteOff() && m.getNoteNumber() == 62)
            setParam (ParamIDs::erase, 0.0f);
        else if (m.isNoteOff() && m.getNoteNumber() == 68)
            setParam (ParamIDs::bSeed, 0.0f);
        else if (m.isController())
        {
            static const char* ccMap[] = { ParamIDs::in, ParamIDs::blur, ParamIDs::drift, ParamIDs::drive,
                                           ParamIDs::supcom, ParamIDs::fb, ParamIDs::mix, ParamIDs::out, ParamIDs::algo,
                                           ParamIDs::bTime, ParamIDs::bSize, ParamIDs::bShape, ParamIDs::bPitch,
                                           ParamIDs::bDensity, ParamIDs::bFeedback, ParamIDs::bDryWet, ParamIDs::bReverb };
            const int cc = m.getControllerNumber();
            if (cc >= 20 && cc <= 36)
                if (auto* p = apvts.getParameter (ccMap[cc - 20]))
                    p->setValueNotifyingHost ((float) m.getControllerValue() / 127.0f);
        }
    }
}

void DerivaProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (! prepared.load() || n == 0) { buffer.clear(); return; }

    handleMidi (midi);

    // memory restored from the project
    const SpinLock::ScopedTryLockType pendingGuard (pendingLock);
    if (pendingGuard.isLocked() && pendingMemoryReady.load())
    {
        auto& mem = engine.memory();
        const size_t count = std::min (mem.size(), pendingMemory.size());
        std::copy (pendingMemory.begin(), pendingMemory.begin() + (std::ptrdiff_t) count, mem.begin());
        engine.memoryWasReplaced();
        pendingMemoryReady.store (false);
    }

    const bool eraseAllParam = pEraseAll->load() > 0.5f;
    if ((eraseAllParam && ! lastEraseAllParam) || eraseAllFromUi.exchange (false))
        engine.requestEraseAll();
    lastEraseAllParam = eraseAllParam;

    deriva::Engine::Params p;
    p.algo = (int) std::lround (pAlgo->load());
    p.inGain = Decibels::decibelsToGain (pIn->load());
    p.blur = pBlur->load();
    p.drift = pDrift->load();
    p.drive = pDrive->load();
    p.supcom = pSup->load();
    p.fb = pFb->load();
    p.mix = pMix->load();
    p.outGain = pOut->load() <= -59.9f ? 0.0f : Decibels::decibelsToGain (pOut->load());
    p.hpfOn = pHpfOn->load() > 0.5f;
    p.hpfFreq = (int) pHpfFreq->load();
    p.lpfOn = pLpfOn->load() > 0.5f;
    p.lpfFreq = (int) pLpfFreq->load();
    p.recOn = pRecOn->load() > 0.5f;
    p.recTarget = (int) pRecTarget->load();
    p.reverse = pReverse->load() > 0.5f;
    p.halfSpeed = pHalf->load() > 0.5f;
    p.erase = pErase->load() > 0.5f;
    p.monoInput = pInputMode->load() > 0.5f;
    p.gateSensitive = pGate->load() > 0.5f;

    const int numIn = getTotalNumInputChannels();

    const bool beadsWanted = pBeadsOn->load() > 0.5f;
    if (! beadsWanted && beadsRamp <= 0.0f)
    {
        // Beads bypassed: plain Deriva
        beadsRunning = false;
        beadsPeak.store (0.0f);
        const float* inL = buffer.getReadPointer (0);
        const float* inR = numIn > 1 ? buffer.getReadPointer (1) : nullptr;
        engine.process (inL, inR, buffer.getWritePointer (0), buffer.getWritePointer (1), n, p);
        return;
    }
    if (! beadsRunning)
    {
        beads.resetStreams();
        std::fill (retFifoL.begin(), retFifoL.end(), 0.0f);
        std::fill (retFifoR.begin(), retFifoR.end(), 0.0f);
        retFifoL.resize ((size_t) capacity); retFifoR.resize ((size_t) capacity);
        retRead = 0;
        beadsRunning = true;
    }

    // host tempo clock (quarter notes) for SYNC
    pulseScratch.clear();
    bool clockRunning = false;
    if (pBSync->load() > 0.5f)
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (pos->getIsPlaying() && pos->getBpm().hasValue() && pos->getPpqPosition().hasValue() && *pos->getBpm() > 1.0)
                {
                    clockRunning = true;
                    const double spb = getSampleRate() * 60.0 / *pos->getBpm();
                    const double ppq = *pos->getPpqPosition();
                    double first = (std::ceil (ppq - 1.0e-9) - ppq) * spb;
                    for (double o = first; o < (double) n; o += spb)
                        pulseScratch.push_back ((int) o);
                }
    beadsClock.store (clockRunning);

    for (int start = 0; start < n; start += capacity)
        processChunk (buffer, start, jmin (capacity, n - start), p, pulseScratch);
}

void DerivaProcessor::processChunk (AudioBuffer<float>& buffer, int start, int len, const deriva::Engine::Params& p,
                                    const std::vector<int>& pulses)
{
    const int numIn = getTotalNumInputChannels();
    const bool loop = pBPlace->load() > 0.5f;
    const int place = loop ? 1 : 0;
    if (place != lastPlace)
    {
        std::fill (retFifoL.begin(), retFifoL.end(), 0.0f);
        std::fill (retFifoR.begin(), retFifoR.end(), 0.0f);
        retFifoL.resize ((size_t) capacity); retFifoR.resize ((size_t) capacity);
        retRead = 0;
        lastPlace = place;
    }

    // enable / disable ramp (30 ms)
    const float target = pBeadsOn->load() > 0.5f ? 1.0f : 0.0f;
    const float step = (float) len / (float) (0.03 * getSampleRate());
    const float r0 = beadsRamp;
    beadsRamp = target > beadsRamp ? jmin (target, beadsRamp + step) : jmax (target, beadsRamp - step);
    const float r1 = beadsRamp;

    deriva::Engine::ExternalIO io;
    io.wetL = wetL.data(); io.wetR = wetR.data(); io.wetGain = wetGain.data();
    if (loop)
    {
        for (int i = 0; i < len; ++i)
        {
            const bool ok = retRead < retFifoL.size();
            retL[(size_t) i] = ok ? retFifoL[retRead] : 0.0f;
            retR[(size_t) i] = ok ? retFifoR[retRead] : 0.0f;
            if (ok) ++retRead;
        }
        io.retL = retL.data(); io.retR = retR.data();
        io.loopMix = pBDryWet->load() * 0.5f * (r0 + r1);
    }

    float* outL = buffer.getWritePointer (0, start);
    float* outR = buffer.getWritePointer (1, start);
    const float* inL = buffer.getReadPointer (0, start);
    const float* inR = numIn > 1 ? buffer.getReadPointer (1, start) : nullptr;
    engine.process (inL, inR, outL, outR, len, p, &io);

    // ---- Beads
    deriva::BeadsUnit::Params bp;
    bp.time = pBTime->load(); bp.timeRnd = pBTimeRnd->load();
    bp.size = pBSize->load(); bp.sizeRnd = pBSizeRnd->load();
    bp.shape = pBShape->load(); bp.shapeRnd = pBShapeRnd->load();
    bp.pitch = pBPitch->load(); bp.pitchRnd = pBPitchRnd->load();
    bp.density = pBDensity->load();
    bp.feedback = loop ? 0.0f : pBFeedback->load();
    bp.dryWet = loop ? 1.0f : pBDryWet->load();
    bp.reverb = loop ? 0.0f : pBReverb->load();
    bp.freeze = pBFreeze->load() > 0.5f;
    bp.seed = pBSeed->load() > 0.5f;
    bp.seedLock = pBSeedLock->load() > 0.5f;
    bp.quality = (int) pBQuality->load();
    bp.mono = pBMono->load() > 0.5f;
    bp.clockPatched = beadsClock.load();

    int local[64]; int nl = 0;
    for (int o : pulses)
        if (o >= start && o < start + len && nl < 64) local[nl++] = o - start;

    std::copy (wetL.begin(), wetL.begin() + len, bL.begin());
    std::copy (wetR.begin(), wetR.begin() + len, bR.begin());
    beads.process (bL.data(), bR.data(), len, bp, local, nl);
    beadsPeak.store (beads.lastOutputPeak());

    if (loop)
    {
        // compact the return queue and append this chunk (used by the next chunks)
        if (retRead > 0)
        {
            retFifoL.erase (retFifoL.begin(), retFifoL.begin() + (std::ptrdiff_t) retRead);
            retFifoR.erase (retFifoR.begin(), retFifoR.begin() + (std::ptrdiff_t) retRead);
            retRead = 0;
        }
        retFifoL.insert (retFifoL.end(), bL.begin(), bL.begin() + len);
        retFifoR.insert (retFifoR.end(), bR.begin(), bR.begin() + len);
        for (int i = 0; i < len; ++i)
        {
            outL[i] += wetGain[(size_t) i] * wetL[(size_t) i];
            outR[i] += wetGain[(size_t) i] * wetR[(size_t) i];
        }
    }
    else
    {
        for (int i = 0; i < len; ++i)
        {
            const float r = r0 + (r1 - r0) * (float) i / (float) len;
            outL[i] += wetGain[(size_t) i] * (wetL[(size_t) i] + r * (bL[(size_t) i] - wetL[(size_t) i]));
            outR[i] += wetGain[(size_t) i] * (wetR[(size_t) i] + r * (bR[(size_t) i] - wetR[(size_t) i]));
        }
    }
    if (beadsRamp <= 0.0f && target <= 0.0f) beadsRunning = false;
}

//==============================================================================
void DerivaProcessor::getStateInformation (MemoryBlock& destData)
{
    MemoryOutputStream mos (destData, false);
    mos.writeInt (kStateMagic);
    if (auto xml = apvts.copyState().createXml())
        mos.writeString (xml->toString());
    else
        mos.writeString ({});

    const bool withMemory = pSaveMem->load() > 0.5f && prepared.load();
    mos.writeBool (withMemory);
    if (withMemory)
    {
        auto& mem = const_cast<deriva::Engine&> (engine).memory();
        mos.writeDouble ((double) engine.sampleRate());
        mos.writeInt ((int) mem.size());

        MemoryBlock compressed;
        {
            MemoryOutputStream cmp (compressed, false);
            GZIPCompressorOutputStream gz (cmp, 6);
            HeapBlock<int16> chunk (8192);
            for (size_t i = 0; i < mem.size(); i += 8192)
            {
                const size_t c = std::min<size_t> (8192, mem.size() - i);
                for (size_t k = 0; k < c; ++k)
                    chunk[k] = (int16) jlimit (-32767, 32767, roundToInt (mem[i + k] * 32767.0f));
                gz.write (chunk.getData(), c * sizeof (int16));
            }
            gz.flush();
        }
        mos.writeInt64 ((int64) compressed.getSize());
        mos.write (compressed.getData(), compressed.getSize());
    }
}

void DerivaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    MemoryInputStream mis (data, (size_t) sizeInBytes, false);
    if (mis.readInt() != kStateMagic)
    {
        // maybe plain XML from a host
        if (auto xml = getXmlFromBinary (data, sizeInBytes))
            apvts.replaceState (ValueTree::fromXml (*xml));
        return;
    }
    if (auto xml = parseXML (mis.readString()))
        apvts.replaceState (ValueTree::fromXml (*xml));

    if (! mis.readBool()) return;
    const double savedRate = mis.readDouble();
    const int savedSize = mis.readInt();
    const int64 csize = mis.readInt64();
    if (savedSize <= 0 || csize <= 0 || csize > mis.getNumBytesRemaining()) return;

    MemoryBlock compressed;
    mis.readIntoMemoryBlock (compressed, (ssize_t) csize);
    MemoryInputStream cin (compressed, false);
    GZIPDecompressorInputStream gz (cin);
    std::vector<float> raw ((size_t) savedSize);
    HeapBlock<int16> chunk (8192);
    for (size_t i = 0; i < raw.size(); i += 8192)
    {
        const size_t c = std::min<size_t> (8192, raw.size() - i);
        const int got = gz.read (chunk.getData(), (int) (c * sizeof (int16)));
        const size_t gotS = (size_t) std::max (0, got) / sizeof (int16);
        for (size_t k = 0; k < c; ++k) raw[i + k] = k < gotS ? (float) chunk[k] / 32767.0f : 0.0f;
    }

    // adapt to the current sample rate (linear resampling) if needed
    const double rate = preparedRate > 0.0 ? preparedRate : savedRate;
    if (std::abs (rate - savedRate) > 1.0)
    {
        const double ratio = savedRate / rate;
        std::vector<float> rs ((size_t) std::ceil (raw.size() / ratio));
        for (size_t i = 0; i < rs.size(); ++i)
        {
            const double pos = (double) i * ratio;
            const size_t a = std::min (raw.size() - 1, (size_t) pos);
            const size_t b = std::min (raw.size() - 1, a + 1);
            const float t = (float) (pos - (double) a);
            rs[i] = raw[a] + t * (raw[b] - raw[a]);
        }
        raw.swap (rs);
    }

    const SpinLock::ScopedLockType guard (pendingLock);
    pendingMemory.swap (raw);
    pendingMemoryReady.store (true);
}

AudioProcessorEditor* DerivaProcessor::createEditor() { return new DerivaEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DerivaProcessor(); }
