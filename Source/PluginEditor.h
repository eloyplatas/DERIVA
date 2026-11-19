#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace ui
{
//==============================================================================
struct Theme
{
    static inline const juce::Colour bgTop { 0xff17171a };
    static inline const juce::Colour bgBottom { 0xff0b0b0d };
    static inline const juce::Colour text { 0xffeeeeef };
    static inline const juce::Colour dim { 0xff8a8a92 };
    static inline const juce::Colour faint { 0xff3a3a40 };
    static inline const juce::Colour accent { 0xff7a5cff };
    static inline const juce::Colour accentLight { 0xffb4a3ff };
    static inline const juce::Colour ledGreen { 0xff3be37f };
    static inline const juce::Colour ledYellow { 0xffffc94a };
    static inline const juce::Colour ledRed { 0xffff4b4b };
    static inline const juce::Colour ledWhite { 0xfff4f0ff };
};

struct Fonts
{
    static juce::Font regular (float h, float kern = 0.0f);
    static juce::Font medium (float h, float kern = 0.0f);
    static juce::Font semibold (float h, float kern = 0.0f);
    static juce::Font bold (float h, float kern = 0.0f);
};

void drawLed (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour col, float brightness);

//==============================================================================
class DerivaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DerivaLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float startA, float endA, juce::Slider&) override;
    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
    void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override;
};

//==============================================================================
struct RoundSlider : public juce::Slider
{
    float hitR = 30.0f;
    bool hitTest (int x, int y) override
    {
        return getLocalBounds().toFloat().getCentre().getDistanceFrom ({ (float) x, (float) y }) <= hitR;
    }
};

class Knob : public juce::Component
{
public:
    enum class Kind { Normal, Small, Feedback, SupCom, Mini, Bipolar };
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& name, Kind, const juce::String& tooltip);
    void resized() override;
    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    float knobRadius() const { return kind == Kind::Small ? 20.0f : (kind == Kind::Mini ? 14.0f : 27.0f); }

private:
    RoundSlider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::RangedAudioParameter* param = nullptr;
    juce::String name;
    Kind kind;
};

//==============================================================================
class CosmosDial : public juce::Component, public juce::SettableTooltipClient
{
public:
    CosmosDial (DerivaProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void updateFromEngine();

    static float positionAngle (int index0); // radians clockwise from 12 o'clock

private:
    void setAlgo (int a);
    int nearestPosition (juce::Point<float> local, float& distance) const;

    DerivaProcessor& proc;
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    int algo = 2;
    float dragAccum = 0.0f;
    bool dragging = false;

    int nLines = 0;
    std::array<float, deriva::kMaxLines> heads {};
    std::array<float, deriva::kMaxLines> lenSec {};
    std::array<std::array<float, deriva::kEnvBins>, deriva::kMaxLines> env {};
    float speed = 1.0f;
};

//==============================================================================
class FootSwitch : public juce::Component, public juce::SettableTooltipClient
{
public:
    enum class LedMode { None, Pair, Single };
    FootSwitch (const juce::String& name, const juce::String& subLabel, LedMode, const juce::String& tooltip);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void tick(); // called by the editor timer (long press detection)

    std::function<void()> onClick, onRightClick, onShiftClick, onPress, onRelease, onLongPress;
    std::function<void (int)> onWheel;
    std::function<std::pair<float, float>()> ledState; // brightness left/right (or single)
    juce::Colour ledColour { Theme::ledWhite };
    juce::uint32 longPressMs = 2000;
    juce::Point<float> switchCentre() const { return { getWidth() * 0.5f, 82.0f }; }

private:
    juce::String name, sub;
    LedMode ledMode;
    bool down = false, longFired = false, shiftDown = false;
    juce::uint32 downTime = 0;
};

//==============================================================================
class Chip : public juce::Component, public juce::SettableTooltipClient
{
public:
    Chip (const juce::String& text, const juce::String& tooltip) : label (text) { setTooltip (tooltip); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { pressed = true; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override { pressed = false; repaint(); if (contains (e.getPosition()) && onClick) onClick(); }
    std::function<void()> onClick;
    std::function<bool()> isOn;
    int idealWidth() const;

private:
    juce::String label;
    bool pressed = false;
};

//==============================================================================
// A row of segments (LED + label) bound to a choice parameter
class ChoiceRow : public juce::Component, public juce::SettableTooltipClient
{
public:
    ChoiceRow (juce::RangedAudioParameter& p, const juce::StringArray& labels, const juce::String& tooltip);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    juce::StringArray labels;
    int index = 0;
};

//==============================================================================
class BeadsPanel : public juce::Component
{
public:
    static constexpr int W = 1040, H = 300;
    explicit BeadsPanel (DerivaProcessor&);
    void paint (juce::Graphics&) override;
    void refresh();

private:
    void buildBackground();
    void setParam (const char* id, float normalised);
    float value (const char* id) const;

    DerivaProcessor& proc;
    juce::Image background;
    Knob time, size, shape, pitch, density, feedback, drywet, reverb;
    Knob timeRnd, sizeRnd, shapeRnd, pitchRnd;
    FootSwitch freeze, seed;
    ChoiceRow quality, place;
    Chip chipMono, chipSync, chipClose;
    float activity = 0.0f;
};

//==============================================================================
class Panel : public juce::Component
{
public:
    static constexpr int W = 1040, H = 520;
    explicit Panel (DerivaProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override {}
    void refresh();

private:
    void buildBackground();
    void setParam (const char* id, float normalised);
    void toggle (const char* id);
    void cycle (const char* id, int delta);
    float value (const char* id) const;

    DerivaProcessor& proc;
    juce::Image background;

    Knob in, blur, drift, drive, supcom, fb, mix, out;
    CosmosDial dial;
    FootSwitch hpf, lpf, erase, reverse, record;
    Chip chipMono, chipGate, chipHalf, chipSave, chipEraseAll, chipBeads;

    float meterLevel = 0.0f, inLevel = 0.0f;
    bool limiterOn = false, inClip = false;
    int clipHold = 0, limHold = 0;
};

} // namespace ui

//==============================================================================
class DerivaEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DerivaEditor (DerivaProcessor&);
    ~DerivaEditor() override;
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }
    void refreshNow() { timerCallback(); }

private:
    void timerCallback() override;

    DerivaProcessor& proc;
    void layoutForState (bool beadsOpen);

    ui::DerivaLookAndFeel lnf;
    juce::Component stack;           // both panels at base size, scaled as a whole
    ui::Panel panel;
    ui::BeadsPanel beadsPanel;
    bool beadsOpen = false;
    juce::TooltipWindow tooltips { this, 700 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DerivaEditor)
};
