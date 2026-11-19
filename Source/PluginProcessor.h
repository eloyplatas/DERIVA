#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "dsp/Engine.h"
#include "dsp/BeadsUnit.h"

namespace ParamIDs
{
    inline constexpr const char* algo = "algo";
    inline constexpr const char* in = "in";
    inline constexpr const char* blur = "blur";
    inline constexpr const char* drift = "drift";
    inline constexpr const char* drive = "drive";
    inline constexpr const char* supcom = "supcom";
    inline constexpr const char* fb = "fb";
    inline constexpr const char* mix = "mix";
    inline constexpr const char* out = "out";
    inline constexpr const char* hpfOn = "hpfOn";
    inline constexpr const char* hpfFreq = "hpfFreq";
    inline constexpr const char* lpfOn = "lpfOn";
    inline constexpr const char* lpfFreq = "lpfFreq";
    inline constexpr const char* recOn = "recOn";
    inline constexpr const char* recTarget = "recTarget";
    inline constexpr const char* reverse = "reverse";
    inline constexpr const char* halfSpeed = "halfSpeed";
    inline constexpr const char* erase = "erase";
    inline constexpr const char* eraseAll = "eraseAll";
    inline constexpr const char* inputMode = "inputMode";
    inline constexpr const char* gate = "gate";
    inline constexpr const char* saveMemory = "saveMemory";
    // Beads stage
    inline constexpr const char* beadsOn = "beadsOn";
    inline constexpr const char* bPlace = "bPlace";
    inline constexpr const char* bTime = "bTime";
    inline constexpr const char* bTimeRnd = "bTimeRnd";
    inline constexpr const char* bSize = "bSize";
    inline constexpr const char* bSizeRnd = "bSizeRnd";
    inline constexpr const char* bShape = "bShape";
    inline constexpr const char* bShapeRnd = "bShapeRnd";
    inline constexpr const char* bPitch = "bPitch";
    inline constexpr const char* bPitchRnd = "bPitchRnd";
    inline constexpr const char* bDensity = "bDensity";
    inline constexpr const char* bFeedback = "bFeedback";
    inline constexpr const char* bDryWet = "bDryWet";
    inline constexpr const char* bReverb = "bReverb";
    inline constexpr const char* bFreeze = "bFreeze";
    inline constexpr const char* bSeed = "bSeed";
    inline constexpr const char* bSeedLock = "bSeedLock";
    inline constexpr const char* bQuality = "bQuality";
    inline constexpr const char* bMono = "bMono";
    inline constexpr const char* bSync = "bSync";
}

class DerivaProcessor : public juce::AudioProcessor
{
public:
    DerivaProcessor();
    ~DerivaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Deriva"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    const deriva::Engine& getEngine() const { return engine; }
    bool isEnginePrepared() const { return prepared.load(); }

    void triggerEraseAll() { eraseAllFromUi.store (true); }
    float beadsActivity() const { return beadsPeak.load(); }
    bool beadsClockRunning() const { return beadsClock.load(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void handleMidi (const juce::MidiBuffer& midi);
    void setParam (const char* id, float plainValue);
    void toggleParam (const char* id);

    void processChunk (juce::AudioBuffer<float>& buffer, int start, int len, const deriva::Engine::Params& p,
                       const std::vector<int>& pulses);

    deriva::Engine engine;
    deriva::BeadsUnit beads;
    int capacity = 0;
    std::vector<float> wetL, wetR, wetGain, bL, bR, retL, retR;
    std::vector<float> retFifoL, retFifoR;   // loop return queue
    size_t retRead = 0;
    float beadsRamp = 0.0f;                  // 0 = bypass, 1 = fully engaged
    bool beadsRunning = false;
    int lastPlace = -1;
    std::atomic<float> beadsPeak { 0.0f };
    std::atomic<bool> beadsClock { false };
    std::vector<int> pulseScratch;
    std::atomic<bool> prepared { false };
    double preparedRate = 0.0;

    std::atomic<bool> eraseAllFromUi { false };
    bool lastEraseAllParam = false;

    // pending memory restored from a saved project (handed to the audio thread)
    std::vector<float> pendingMemory;
    std::array<double, deriva::kMaxLines> pendingHeads {};
    std::atomic<bool> pendingMemoryReady { false };
    juce::SpinLock pendingLock;

    // raw parameter pointers
    std::atomic<float>* pAlgo = nullptr; std::atomic<float>* pIn = nullptr; std::atomic<float>* pBlur = nullptr;
    std::atomic<float>* pDrift = nullptr; std::atomic<float>* pDrive = nullptr; std::atomic<float>* pSup = nullptr;
    std::atomic<float>* pFb = nullptr; std::atomic<float>* pMix = nullptr; std::atomic<float>* pOut = nullptr;
    std::atomic<float>* pHpfOn = nullptr; std::atomic<float>* pHpfFreq = nullptr; std::atomic<float>* pLpfOn = nullptr;
    std::atomic<float>* pLpfFreq = nullptr; std::atomic<float>* pRecOn = nullptr; std::atomic<float>* pRecTarget = nullptr;
    std::atomic<float>* pReverse = nullptr; std::atomic<float>* pHalf = nullptr; std::atomic<float>* pErase = nullptr;
    std::atomic<float>* pEraseAll = nullptr; std::atomic<float>* pInputMode = nullptr; std::atomic<float>* pGate = nullptr;
    std::atomic<float>* pSaveMem = nullptr;
    std::atomic<float>* pBeadsOn = nullptr; std::atomic<float>* pBPlace = nullptr;
    std::atomic<float>* pBTime = nullptr; std::atomic<float>* pBTimeRnd = nullptr;
    std::atomic<float>* pBSize = nullptr; std::atomic<float>* pBSizeRnd = nullptr;
    std::atomic<float>* pBShape = nullptr; std::atomic<float>* pBShapeRnd = nullptr;
    std::atomic<float>* pBPitch = nullptr; std::atomic<float>* pBPitchRnd = nullptr;
    std::atomic<float>* pBDensity = nullptr; std::atomic<float>* pBFeedback = nullptr;
    std::atomic<float>* pBDryWet = nullptr; std::atomic<float>* pBReverb = nullptr;
    std::atomic<float>* pBFreeze = nullptr; std::atomic<float>* pBSeed = nullptr; std::atomic<float>* pBSeedLock = nullptr;
    std::atomic<float>* pBQuality = nullptr; std::atomic<float>* pBMono = nullptr; std::atomic<float>* pBSync = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DerivaProcessor)
};
