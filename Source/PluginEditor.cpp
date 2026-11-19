#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

namespace ui
{
//==============================================================================
// Fonts (Montserrat, SIL Open Font License)
namespace
{
    Typeface::Ptr loadTf (const char* data, int size) { return Typeface::createSystemTypefaceFor (data, (size_t) size); }
    Typeface::Ptr tfRegular()  { static auto t = loadTf (BinaryData::MontserratRegular_ttf, BinaryData::MontserratRegular_ttfSize); return t; }
    Typeface::Ptr tfMedium()   { static auto t = loadTf (BinaryData::MontserratMedium_ttf, BinaryData::MontserratMedium_ttfSize); return t; }
    Typeface::Ptr tfSemibold() { static auto t = loadTf (BinaryData::MontserratSemiBold_ttf, BinaryData::MontserratSemiBold_ttfSize); return t; }
    Typeface::Ptr tfBold()     { static auto t = loadTf (BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize); return t; }

    Font makeFont (Typeface::Ptr tf, float h, float kern)
    {
        Font f (FontOptions().withTypeface (tf).withHeight (h));
        if (kern != 0.0f) f = f.withExtraKerningFactor (kern);
        return f;
    }

    constexpr float kRotStart = MathConstants<float>::pi * 1.25f;
    constexpr float kRotEnd = MathConstants<float>::pi * 2.75f;
}

Font Fonts::regular (float h, float k)  { return makeFont (tfRegular(), h, k); }
Font Fonts::medium (float h, float k)   { return makeFont (tfMedium(), h, k); }
Font Fonts::semibold (float h, float k) { return makeFont (tfSemibold(), h, k); }
Font Fonts::bold (float h, float k)     { return makeFont (tfBold(), h, k); }

//==============================================================================
void drawLed (Graphics& g, Point<float> c, float r, Colour col, float b)
{
    b = jlimit (0.0f, 1.0f, b);
    g.setColour (Colour (0xff050506));
    g.fillEllipse (c.x - r - 1.3f, c.y - r - 1.3f, 2.0f * (r + 1.3f), 2.0f * (r + 1.3f));
    if (b > 0.02f)
    {
        ColourGradient glow (col.withAlpha (0.42f * b), c.x, c.y, col.withAlpha (0.0f), c.x + r * 4.5f, c.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (c.x - r * 4.5f, c.y - r * 4.5f, r * 9.0f, r * 9.0f);
    }
    const auto body = Colour (0xff26262b).interpolatedWith (col, b);
    ColourGradient lens (body.brighter (0.15f + 0.45f * b), c.x - r * 0.3f, c.y - r * 0.4f, body.darker (0.35f), c.x + r, c.y + r, true);
    g.setGradientFill (lens);
    g.fillEllipse (c.x - r, c.y - r, 2.0f * r, 2.0f * r);
    g.setColour (Colours::white.withAlpha (0.18f + 0.4f * b));
    g.fillEllipse (c.x - r * 0.5f, c.y - r * 0.6f, r * 0.62f, r * 0.45f);
}

//==============================================================================
DerivaLookAndFeel::DerivaLookAndFeel()
{
    setColour (TooltipWindow::textColourId, Theme::text);
    setColour (TooltipWindow::backgroundColourId, Colour (0xf0141418));
    setColour (TooltipWindow::outlineColourId, Theme::accent.withAlpha (0.6f));
}

Typeface::Ptr DerivaLookAndFeel::getTypefaceForFont (const Font&) { return tfMedium(); }

void DerivaLookAndFeel::drawTooltip (Graphics& g, const String& text, int width, int height)
{
    auto r = Rectangle<float> (0, 0, (float) width, (float) height);
    g.setColour (Colour (0xf2121216));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (Theme::accent.withAlpha (0.55f));
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    g.setColour (Theme::text);
    g.setFont (Fonts::medium (12.5f));
    g.drawFittedText (text, r.reduced (10, 6).toNearestInt(), Justification::centredLeft, 4);
}

Rectangle<int> DerivaLookAndFeel::getTooltipBounds (const String& tipText, Point<int> screenPos, Rectangle<int> parentArea)
{
    const auto f = Fonts::medium (12.5f);
    const int maxW = 340;
    const int textW = jmin (maxW, (int) GlyphArrangement::getStringWidth (f, tipText) + 24);
    const int lines = jmax (1, (int) std::ceil ((GlyphArrangement::getStringWidth (f, tipText) + 24.0f) / (float) maxW));
    const int w = textW, h = 14 + lines * 17;
    return Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                           screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6, w, h)
        .constrainedWithin (parentArea);
}

void DerivaLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    const int kind = (int) s.getProperties()["kind"];
    const float R = (float) s.getProperties()["radius"];
    const auto area = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto c = area.getCentre();
    const float ang = a0 + pos * (a1 - a0);

    const bool bipolar = kind == 3 || kind == 4 || kind == 5;
    const bool mini = kind == 4;
    // dot scale around the knob, like the printed panel
    for (int i = 0; i < 7; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 6.0f;
        const auto p = c.getPointOnCircumference (R + (mini ? 7.5f : 10.0f), a);
        float dr = (i == 0 || i == 6 || (bipolar && i == 3)) ? 1.8f : 1.25f;
        if (mini) dr *= 0.8f;
        g.setColour (Theme::text.withAlpha (0.8f));
        g.fillEllipse (p.x - dr, p.y - dr, dr * 2.0f, dr * 2.0f);
    }

    // value arc
    {
        float from = a0, to = ang;
        if (bipolar) from = 0.5f * (a0 + a1);
        if (std::abs (to - from) > 0.002f)
        {
            Path arc;
            const float ra = R + (mini ? 3.5f : 5.0f);
            arc.addCentredArc (c.x, c.y, ra, ra, 0.0f, jmin (from, to), jmax (from, to), true);
            g.setColour (Theme::accent);
            g.strokePath (arc, PathStrokeType (mini ? 1.6f : 2.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
    }

    // unity feedback mark
    if (kind == 2)
    {
        const float aU = a0 + (1.0f / 1.15f) * (a1 - a0);
        g.setColour (Theme::accentLight);
        g.drawLine (Line<float> (c.getPointOnCircumference (R + 14.0f, aU), c.getPointOnCircumference (R + 20.0f, aU)), 2.0f);
    }

    // drop shadow
    for (int i = 5; i > 0; --i)
    {
        const float rr = R + (float) i * 1.5f;
        g.setColour (Colours::black.withAlpha (0.10f));
        g.fillEllipse (c.x - rr, c.y - rr + R * 0.12f, rr * 2.0f, rr * 2.0f);
    }

    // knurled black skirt
    ColourGradient skirt (Colour (0xff36363c), c.x - R * 0.5f, c.y - R * 0.8f, Colour (0xff08080a), c.x + R * 0.6f, c.y + R, false);
    g.setGradientFill (skirt);
    g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    g.setColour (Colours::white.withAlpha (0.055f));
    for (int i = 0; i < 48; ++i)
    {
        const float a = (float) i * MathConstants<float>::twoPi / 48.0f;
        g.drawLine (Line<float> (c.getPointOnCircumference (R * 0.80f, a), c.getPointOnCircumference (R * 0.97f, a)), 1.0f);
    }
    g.setColour (Colours::white.withAlpha (0.10f));
    g.drawEllipse (c.x - R + 0.5f, c.y - R + 0.5f, R * 2.0f - 1.0f, R * 2.0f - 1.0f, 1.0f);

    // white top cap with black pointer
    const float rc = R * 0.62f;
    ColourGradient cap (Colour (0xffffffff), c.x - rc * 0.5f, c.y - rc * 0.8f, Colour (0xffb9b9bf), c.x + rc * 0.6f, c.y + rc, false);
    g.setGradientFill (cap);
    g.fillEllipse (c.x - rc, c.y - rc, rc * 2.0f, rc * 2.0f);
    g.setColour (Colours::black.withAlpha (0.35f));
    g.drawEllipse (c.x - rc, c.y - rc, rc * 2.0f, rc * 2.0f, 1.0f);
    g.setColour (Colour (0xff111114));
    g.drawLine (Line<float> (c.getPointOnCircumference (rc * 0.15f, ang), c.getPointOnCircumference (rc * 0.9f, ang)), 2.6f);
    g.setColour (Colours::white.withAlpha (0.9f));
    g.drawLine (Line<float> (c.getPointOnCircumference (R * 0.71f, ang), c.getPointOnCircumference (R * 0.95f, ang)), 2.0f);
}

//==============================================================================
Knob::Knob (AudioProcessorValueTreeState& state, const String& paramId, const String& n, Kind k, const String& tip)
    : name (n), kind (k)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (kRotStart, kRotEnd, true);
    slider.setMouseDragSensitivity (240);
    slider.getProperties().set ("kind", (int) kind);
    slider.getProperties().set ("radius", (double) knobRadius());
    slider.setTooltip (tip);
    slider.hitR = knobRadius() + 12.0f;
    slider.addMouseListener (this, false);
    slider.onValueChange = [this] { repaint(); };
    addAndMakeVisible (slider);
    attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);
    param = state.getParameter (paramId);
    if (param != nullptr)
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    setInterceptsMouseClicks (false, true);
}

void Knob::resized() { slider.setBounds (getLocalBounds()); }

void Knob::paint (Graphics& g)
{
    const float R = knobRadius();
    const auto c = getLocalBounds().toFloat().getCentre();
    const bool active = slider.isMouseOverOrDragging();
    const bool mini = kind == Kind::Mini;
    const auto labelArea = Rectangle<float> (0.0f, c.y + R + (mini ? 9.0f : 13.0f), (float) getWidth(), 16.0f);
    if (active && param != nullptr)
    {
        g.setColour (Theme::accentLight);
        g.setFont (Fonts::semibold (mini ? 9.0f : (kind == Kind::Small ? 10.5f : 11.5f), 0.06f));
        g.drawText (param->getCurrentValueAsText(), labelArea, Justification::centred);
    }
    else
    {
        g.setColour (mini ? Theme::dim : Theme::text);
        g.setFont (Fonts::semibold (mini ? 8.5f : (kind == Kind::Small ? 11.0f : 12.5f), mini ? 0.22f : 0.16f));
        g.drawText (name, labelArea, Justification::centred);
    }

    if (kind == Kind::SupCom)
    {
        g.setColour (Theme::dim);
        g.setFont (Fonts::semibold (8.5f, 0.12f));
        const auto pl = c.getPointOnCircumference (R + 22.0f, kRotStart);
        const auto pr = c.getPointOnCircumference (R + 22.0f, kRotEnd);
        g.drawText ("SUP", Rectangle<float> (pl.x - 16, pl.y - 6, 32, 12), Justification::centred);
        g.drawText ("COM", Rectangle<float> (pr.x - 16, pr.y - 6, 32, 12), Justification::centred);
    }
    if (kind == Kind::Feedback)
    {
        const float aU = kRotStart + (1.0f / 1.15f) * (kRotEnd - kRotStart);
        const auto p = c.getPointOnCircumference (R + 28.0f, aU);
        g.setColour (Theme::accentLight);
        g.setFont (Fonts::semibold (8.5f, 0.05f));
        g.drawText ("1.0", Rectangle<float> (p.x - 12, p.y - 6, 24, 12), Justification::centred);
    }
}

//==============================================================================
float CosmosDial::positionAngle (int k)
{
    return degreesToRadians (-150.0f + (float) k * 300.0f / 11.0f);
}

CosmosDial::CosmosDial (DerivaProcessor& p)
    : proc (p),
      param (*p.apvts.getParameter (ParamIDs::algo)),
      attachment (param, [this] (float v) { algo = jlimit (1, 12, (int) std::lround (v)); repaint(); }, nullptr)
{
    attachment.sendInitialUpdate();
    setTooltip ("Algoritmo: 2 delays, 4 delays, reverb gigante y granular, cada uno en S / M / L. "
                "Clic en un punto, arrastra el selector o usa la rueda.");
}

void CosmosDial::setAlgo (int a)
{
    a = jlimit (1, 12, a);
    if (a != algo) attachment.setValueAsCompleteGesture ((float) a);
}

int CosmosDial::nearestPosition (Point<float> local, float& distance) const
{
    const auto c = getLocalBounds().toFloat().getCentre();
    int best = 0;
    distance = 1.0e9f;
    for (int k = 0; k < 12; ++k)
    {
        const float d = c.getPointOnCircumference (72.0f, positionAngle (k)).getDistanceFrom (local);
        if (d < distance) { distance = d; best = k; }
    }
    return best;
}

void CosmosDial::mouseDown (const MouseEvent& e)
{
    float d;
    const int k = nearestPosition (e.position, d);
    if (d < 13.0f) { setAlgo (k + 1); dragging = false; return; }
    dragging = e.position.getDistanceFrom (getLocalBounds().toFloat().getCentre()) < 100.0f;
}

void CosmosDial::mouseDrag (const MouseEvent& e)
{
    if (! dragging) return;
    const auto c = getLocalBounds().toFloat().getCentre();
    const float a = std::atan2 (e.position.x - c.x, -(e.position.y - c.y)); // clockwise from 12 o'clock
    const float deg = radiansToDegrees (a);
    const int k = jlimit (0, 11, (int) std::lround ((deg + 150.0f) * 11.0f / 300.0f));
    setAlgo (k + 1);
}

void CosmosDial::mouseUp (const MouseEvent&) { dragging = false; }

void CosmosDial::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w)
{
    const float d = w.deltaY != 0.0f ? w.deltaY : w.deltaX;
    if (d > 0) setAlgo (algo + 1);
    else if (d < 0) setAlgo (algo - 1);
}

void CosmosDial::updateFromEngine()
{
    if (! proc.isEnginePrepared()) { nLines = 0; return; }
    const auto& e = proc.getEngine();
    nLines = jlimit (0, deriva::kMaxLines, e.numLinesNow());
    speed = e.speedNow();
    for (int i = 0; i < nLines; ++i)
    {
        heads[(size_t) i] = e.headFraction (i);
        lenSec[(size_t) i] = e.lineLengthSeconds (i);
        const float* src = e.lineEnvelope (i);
        std::copy (src, src + deriva::kEnvBins, env[(size_t) i].begin());
    }
    repaint();
}

void CosmosDial::paint (Graphics& g)
{
    const auto c = getLocalBounds().toFloat().getCentre();
    const float twoPi = MathConstants<float>::twoPi;
    const bool reversed = proc.apvts.getRawParameterValue (ParamIDs::reverse)->load() > 0.5f;

    // ---- memory orbits: one ring per delay line, envelope of its content and its head
    const float rMin = 104.0f, rMax = 148.0f;
    for (int i = 0; i < nLines; ++i)
    {
        const float span = (rMax - rMin) / (float) nLines;
        const float r = rMin + span * ((float) i + 0.5f);
        const float hmax = jmin (9.0f, span * 0.46f);
        g.setColour (Theme::text.withAlpha (0.07f));
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.0f);

        const float head = heads[(size_t) i];
        const float barW = jlimit (1.0f, 2.2f, twoPi * r / (float) deriva::kEnvBins * 0.55f);
        for (int b = 0; b < deriva::kEnvBins; ++b)
        {
            const float v = env[(size_t) i][(size_t) b];
            if (v < 2.0e-4f) continue;
            const float frac = ((float) b + 0.5f) / (float) deriva::kEnvBins;
            float d = reversed ? frac - head : head - frac;
            d -= std::floor (d);
            const float recency = std::exp (-d * 5.0f);
            const float hgt = jmax (0.6f, hmax * jlimit (0.0f, 1.0f, std::sqrt (v * 2.2f)));
            const float a = frac * twoPi;
            g.setColour (Theme::text.interpolatedWith (Theme::accentLight, recency).withAlpha (0.20f + 0.65f * recency));
            g.drawLine (Line<float> (c.getPointOnCircumference (r - hgt, a), c.getPointOnCircumference (r + hgt, a)), barW);
        }

        const auto hp = c.getPointOnCircumference (r, head * twoPi);
        ColourGradient glow (Theme::accent.withAlpha (0.55f), hp.x, hp.y, Theme::accent.withAlpha (0.0f), hp.x + 9.0f, hp.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (hp.x - 9.0f, hp.y - 9.0f, 18.0f, 18.0f);
        g.setColour (Theme::ledWhite);
        g.fillEllipse (hp.x - 2.4f, hp.y - 2.4f, 4.8f, 4.8f);
    }

    // ---- algorithm family marks (hatched / wavy / dotted arcs)
    g.setColour (Theme::text.withAlpha (0.6f));
    for (int grp = 0; grp < 4; ++grp)
    {
        const float aS = positionAngle (grp * 3) - 0.10f;
        const float aE = positionAngle (grp * 3 + 2) + 0.10f;
        if (grp == 0 || grp == 1)
        {
            const float step = grp == 0 ? 0.075f : 0.042f;
            for (float a = aS; a <= aE + 1e-4f; a += step)
                g.drawLine (Line<float> (c.getPointOnCircumference (84.0f, a), c.getPointOnCircumference (90.0f, a)), 1.1f);
        }
        else if (grp == 2)
        {
            Path wave;
            const int nPts = 60;
            for (int k = 0; k <= nPts; ++k)
            {
                const float a = aS + (aE - aS) * (float) k / (float) nPts;
                const float r = 87.0f + 2.6f * std::sin ((float) k * 0.75f);
                const auto p = c.getPointOnCircumference (r, a);
                if (k == 0) wave.startNewSubPath (p); else wave.lineTo (p);
            }
            g.strokePath (wave, PathStrokeType (1.1f));
        }
        else
        {
            int k = 0;
            for (float a = aS; a <= aE + 1e-4f; a += 0.055f, ++k)
            {
                const auto p = c.getPointOnCircumference ((k % 2) ? 84.5f : 89.5f, a);
                g.fillEllipse (p.x - 1.3f, p.y - 1.3f, 2.6f, 2.6f);
            }
        }
    }

    // ---- position dots (3 sizes per family)
    for (int k = 0; k < 12; ++k)
    {
        const auto p = c.getPointOnCircumference (72.0f, positionAngle (k));
        const float r = (k % 3 == 0) ? 2.0f : (k % 3 == 1 ? 3.0f : 4.3f);
        if (k + 1 == algo)
        {
            ColourGradient glow (Theme::accent.withAlpha (0.7f), p.x, p.y, Theme::accent.withAlpha (0.0f), p.x + 12.0f, p.y, true);
            g.setGradientFill (glow);
            g.fillEllipse (p.x - 12.0f, p.y - 12.0f, 24.0f, 24.0f);
            g.setColour (Theme::accentLight);
            g.fillEllipse (p.x - r - 0.8f, p.y - r - 0.8f, (r + 0.8f) * 2.0f, (r + 0.8f) * 2.0f);
        }
        else
        {
            g.setColour (Theme::text.withAlpha (0.92f));
            g.fillEllipse (p.x - r, p.y - r, r * 2.0f, r * 2.0f);
        }
    }

    // ---- the big pointer knob
    const float R = 36.0f;
    const float ang = positionAngle (algo - 1);
    for (int i = 6; i > 0; --i)
    {
        const float rr = R + (float) i * 1.8f;
        g.setColour (Colours::black.withAlpha (0.10f));
        g.fillEllipse (c.x - rr, c.y - rr + 4.0f, rr * 2.0f, rr * 2.0f);
    }
    ColourGradient body (Colour (0xff303036), c.x - R * 0.5f, c.y - R * 0.8f, Colour (0xff070708), c.x + R * 0.6f, c.y + R, false);
    g.setGradientFill (body);
    g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    g.setColour (Colours::white.withAlpha (0.05f));
    for (int i = 0; i < 64; ++i)
    {
        const float a = (float) i * twoPi / 64.0f;
        g.drawLine (Line<float> (c.getPointOnCircumference (R * 0.84f, a), c.getPointOnCircumference (R * 0.98f, a)), 1.0f);
    }

    Path bar;
    bar.addRoundedRectangle (-10.0f, -R * 1.18f, 20.0f, R * 1.95f, 8.0f);
    bar.applyTransform (AffineTransform::rotation (ang).translated (c.x, c.y));
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillPath (bar, AffineTransform::translation (0.0f, 3.0f));
    ColourGradient barFill (Colour (0xff3a3a41), c.x - R, c.y - R, Colour (0xff111114), c.x + R, c.y + R, false);
    g.setGradientFill (barFill);
    g.fillPath (bar);
    g.setColour (Colours::white.withAlpha (0.13f));
    g.strokePath (bar, PathStrokeType (1.0f));
    g.setColour (Colours::white);
    g.drawLine (Line<float> (c.getPointOnCircumference (R * 0.30f, ang), c.getPointOnCircumference (R * 1.08f, ang)), 2.4f);
    ColourGradient capG (Colour (0xff45454c), c.x - 6, c.y - 6, Colour (0xff141416), c.x + 6, c.y + 6, false);
    g.setGradientFill (capG);
    g.fillEllipse (c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f);
}

//==============================================================================
FootSwitch::FootSwitch (const String& n, const String& s, LedMode m, const String& tip)
    : name (n), sub (s), ledMode (m)
{
    setTooltip (tip);
}

void FootSwitch::paint (Graphics& g)
{
    const float cx = (float) getWidth() * 0.5f;
    if (ledMode != LedMode::None && ledState)
    {
        const auto st = ledState();
        const auto col = ledColour;
        if (ledMode == LedMode::Pair)
        {
            drawLed (g, { cx - 7.5f, 10.0f }, 3.3f, col, st.first);
            drawLed (g, { cx + 7.5f, 10.0f }, 3.3f, col, st.second);
        }
        else drawLed (g, { cx, 10.0f }, 3.3f, col, st.first);
    }

    g.setColour (Theme::text);
    g.setFont (Fonts::semibold (12.5f, 0.16f));
    g.drawText (name, Rectangle<float> (0, 20, (float) getWidth(), 18), Justification::centred);

    const auto c = switchCentre();
    // shadow
    for (int i = 5; i > 0; --i)
    {
        g.setColour (Colours::black.withAlpha (0.10f));
        const float rr = 26.0f + (float) i * 1.6f;
        g.fillEllipse (c.x - rr, c.y - rr + 4.0f, rr * 2.0f, rr * 2.0f);
    }
    // hex nut
    Path nut;
    nut.addPolygon (c, 6, 27.0f, MathConstants<float>::pi / 6.0f);
    ColourGradient nutG (Colour (0xffe6e6ea), c.x - 20, c.y - 24, Colour (0xff5d5d63), c.x + 18, c.y + 26, false);
    nutG.addColour (0.5, Colour (0xffa9a9b0));
    g.setGradientFill (nutG);
    g.fillPath (nut);
    g.setColour (Colours::black.withAlpha (0.35f));
    g.strokePath (nut, PathStrokeType (1.0f));
    // gap
    g.setColour (Colour (0xff1a1a1d));
    g.fillEllipse (c.x - 19.0f, c.y - 19.0f, 38.0f, 38.0f);
    // chrome dome
    const float off = down ? 1.0f : 0.0f;
    const float rd = down ? 16.0f : 17.0f;
    ColourGradient dome (down ? Colour (0xffcfcfd4) : Colour (0xfffbfbfd), c.x - 6, c.y - 8 + off, down ? Colour (0xff55555b) : Colour (0xff6c6c73), c.x + rd, c.y + rd + off, true);
    g.setGradientFill (dome);
    g.fillEllipse (c.x - rd, c.y - rd + off, rd * 2.0f, rd * 2.0f);
    g.setColour (Colours::white.withAlpha (down ? 0.25f : 0.55f));
    g.fillEllipse (c.x - 9.0f, c.y - 12.0f + off, 11.0f, 6.0f);
    g.setColour (Colours::black.withAlpha (0.4f));
    g.drawEllipse (c.x - rd, c.y - rd + off, rd * 2.0f, rd * 2.0f, 1.0f);

    if (sub.isNotEmpty())
    {
        g.setColour (Theme::dim);
        g.setFont (Fonts::semibold (8.5f, 0.18f));
        g.drawText (sub, Rectangle<float> (0, 113, (float) getWidth(), 12), Justification::centred);
    }
}

void FootSwitch::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { if (onRightClick) onRightClick(); return; }
    shiftDown = e.mods.isShiftDown();
    down = true;
    longFired = false;
    downTime = Time::getMillisecondCounter();
    if (onPress && ! shiftDown) onPress();
    repaint();
}

void FootSwitch::mouseUp (const MouseEvent& e)
{
    if (! down) return;
    down = false;
    if (onRelease) onRelease();
    if (! longFired && getLocalBounds().contains (e.getPosition()))
    {
        if (shiftDown && onShiftClick) onShiftClick();
        else if (onClick) onClick();
    }
    repaint();
}

void FootSwitch::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w)
{
    const float d = w.deltaY != 0.0f ? w.deltaY : w.deltaX;
    if (onWheel && d != 0.0f) onWheel (d > 0 ? 1 : -1);
}

void FootSwitch::tick()
{
    if (down && ! longFired && onLongPress && Time::getMillisecondCounter() - downTime > longPressMs)
    {
        longFired = true;
        onLongPress();
    }
    repaint();
}

//==============================================================================
int Chip::idealWidth() const
{
    return (int) GlyphArrangement::getStringWidth (Fonts::semibold (9.5f, 0.14f), label) + 24;
}

void Chip::paint (Graphics& g)
{
    const bool on = isOn ? isOn() : false;
    const auto r = getLocalBounds().toFloat().reduced (0.5f);
    const float rad = r.getHeight() * 0.5f;
    if (on || pressed)
    {
        g.setColour (Theme::accent.withAlpha (pressed ? 0.45f : 0.28f));
        g.fillRoundedRectangle (r, rad);
    }
    g.setColour (on ? Theme::accent : Theme::text.withAlpha (0.2f));
    g.drawRoundedRectangle (r, rad, 1.0f);
    g.setColour (on ? Theme::text : Theme::dim);
    g.setFont (Fonts::semibold (9.5f, 0.14f));
    g.drawText (label, r, Justification::centred);
}

//==============================================================================
namespace
{
    struct KnobSpec { float cx, cy; };
    // a 50 px header bar holds the logo and the chips; everything else sits below it
    constexpr float kHeader = 50.0f;
    constexpr KnobSpec kIn { 74, 166 + kHeader }, kBlur { 150, 280 + kHeader }, kDrift { 244, 230 + kHeader }, kDrive { 332, 178 + kHeader },
                       kSup { 708, 178 + kHeader }, kFb { 796, 230 + kHeader }, kMix { 890, 280 + kHeader }, kOut { 966, 166 + kHeader };
    constexpr Point<float> kDialCentre { 520.0f, 184.0f + kHeader };
    constexpr float kSwitchX[5] = { 120, 320, 520, 720, 920 };
    constexpr float kSwitchTop = 334.0f + kHeader;
    constexpr float kMeterY = 346.0f + kHeader;
}

Panel::Panel (DerivaProcessor& p)
    : proc (p),
      in (p.apvts, ParamIDs::in, "IN", Knob::Kind::Small, "Nivel de entrada. El LED se pone rojo si la entrada satura."),
      blur (p.apvts, ParamIDs::blur, "BLUR", Knob::Kind::Normal, "Delays: realimentación cruzada entre líneas (difumina su contenido). Granular: mueve la posición de los granos con ruido. Reverb: sin efecto."),
      drift (p.apvts, ParamIDs::drift, "DRIFT", Knob::Kind::Normal, "Delays: paneo de cada línea con un LFO asíncrono cuya velocidad modula una señal caótica (Lorenz). Granular: posición de los granos. Reverb: sin efecto."),
      drive (p.apvts, ParamIDs::drive, "DRIVE", Knob::Kind::Normal, "Saturación suave en la entrada a la memoria (tanh con antialiasing ADAA)."),
      supcom (p.apvts, ParamIDs::supcom, "SUP | COM", Knob::Kind::SupCom, "Izquierda: supresor, lo que entra sustituye lo grabado. Centro: apagado. Derecha: compresor controlado por memoria + entrada."),
      fb (p.apvts, ParamIDs::fb, "FB", Knob::Kind::Feedback, "Realimentación. Por debajo de 1.0 se desvanece, en 1.0 se mantiene y por encima crece hasta el limitador."),
      mix (p.apvts, ParamIDs::mix, "MIX", Knob::Kind::Normal, "Balance seco / memoria. En el centro ambos salen a 0 dB."),
      out (p.apvts, ParamIDs::out, "OUT", Knob::Kind::Small, "Nivel de salida."),
      dial (p),
      hpf ("HPF", "", FootSwitch::LedMode::Pair, "Clic: activa o desactiva. Clic derecho o rueda: frecuencia de corte (LEDs: MIN / MED / MAX)."),
      lpf ("LPF", "", FootSwitch::LedMode::Pair, "Clic: activa o desactiva. Clic derecho o rueda: frecuencia de corte (MIN es la más oscura)."),
      erase ("ERASE", "SHIFT + CLIC: ERASE ALL", FootSwitch::LedMode::None, "Mantén pulsado para borrar en la posición de los cabezales. Mayús + clic borra toda la memoria."),
      reverse ("REVERSE", "SELECT", FootSwitch::LedMode::Single, "Reproducción inversa: invierte lo grabado y lo que entra, como dar la vuelta a la cinta."),
      record ("RECORD", "HOLD 2 S: HALF SPEED", FootSwitch::LedMode::Pair, "Clic: grabar sí / no. Clic derecho o rueda: grupo 1 / grupo 2 / todos. Mantén 2 s: media velocidad (delays)."),
      chipMono ("MONO IN", "Suma las dos entradas y las manda a ambos grupos."),
      chipGate ("GATE SENS", "Noise gate más sensible (umbral 20 dB más bajo) para no comerse ataques suaves."),
      chipHalf ("1/2 SPEED", "Media velocidad: lo grabado suena una octava abajo (solo algoritmos de delays)."),
      chipSave ("SAVE MEMORY", "Guarda el contenido de la memoria dentro del proyecto (aumenta su tamaño)."),
      chipEraseAll ("ERASE ALL", "Borra toda la memoria al instante."),
      chipBeads ("BEADS", "Abre el panel BEADS y activa la etapa granular de Mutable Instruments sobre la memoria. Vuelve a pulsar para cerrarlo y desactivarla.")
{
    for (auto* k : { &in, &blur, &drift, &drive, &supcom, &fb, &mix, &out })
        addAndMakeVisible (k);
    addAndMakeVisible (dial);
    for (auto* f : { &hpf, &lpf, &erase, &reverse, &record })
        addAndMakeVisible (f);
    for (auto* c : { &chipMono, &chipGate, &chipHalf, &chipSave, &chipEraseAll, &chipBeads })
        addAndMakeVisible (c);
    record.ledColour = Theme::ledRed;

    auto place = [] (Knob& k, KnobSpec s)
    {
        const int S = (int) (k.knobRadius() * 2.0f + 64.0f);
        k.setBounds ((int) s.cx - S / 2, (int) s.cy - S / 2, S, S);
    };
    place (in, kIn); place (blur, kBlur); place (drift, kDrift); place (drive, kDrive);
    place (supcom, kSup); place (fb, kFb); place (mix, kMix); place (out, kOut);

    dial.setBounds ((int) kDialCentre.x - 160, (int) kDialCentre.y - 160, 320, 320);

    FootSwitch* sw[5] = { &hpf, &lpf, &erase, &reverse, &record };
    for (int i = 0; i < 5; ++i)
        sw[i]->setBounds ((int) kSwitchX[i] - 70, (int) kSwitchTop, 140, 130);

    // chips, right-aligned on the top edge
    {
        int x = W - 30;
        Chip* chips[5] = { &chipEraseAll, &chipSave, &chipHalf, &chipGate, &chipMono };
        for (auto* c : chips)
        {
            const int w = c->idealWidth();
            x -= w;
            c->setBounds (x, 14, w, 22);
            x -= 8;
        }
        const int wb = chipBeads.idealWidth() + 22;
        x -= 14 + wb;
        chipBeads.setBounds (x, 12, wb, 26);
    }

    // ---- behaviour
    hpf.onClick = [this] { toggle (ParamIDs::hpfOn); };
    hpf.onRightClick = [this] { cycle (ParamIDs::hpfFreq, 1); };
    hpf.onWheel = [this] (int d) { cycle (ParamIDs::hpfFreq, d); };
    hpf.ledState = [this] {
        const float b = value (ParamIDs::hpfOn) > 0.5f ? 1.0f : 0.2f;
        const int f = (int) std::lround (value (ParamIDs::hpfFreq) * 2.0f);
        return std::pair<float, float> { f != 1 ? b : 0.0f, f != 0 ? b : 0.0f };
    };
    lpf.onClick = [this] { toggle (ParamIDs::lpfOn); };
    lpf.onRightClick = [this] { cycle (ParamIDs::lpfFreq, 1); };
    lpf.onWheel = [this] (int d) { cycle (ParamIDs::lpfFreq, d); };
    lpf.ledState = [this] {
        const float b = value (ParamIDs::lpfOn) > 0.5f ? 1.0f : 0.2f;
        const int f = (int) std::lround (value (ParamIDs::lpfFreq) * 2.0f);
        return std::pair<float, float> { f != 1 ? b : 0.0f, f != 0 ? b : 0.0f };
    };
    erase.onPress = [this] { setParam (ParamIDs::erase, 1.0f); };
    erase.onRelease = [this] { setParam (ParamIDs::erase, 0.0f); };
    erase.onShiftClick = [this] { proc.triggerEraseAll(); };
    reverse.onClick = [this] { toggle (ParamIDs::reverse); };
    reverse.ledState = [this] { const float b = value (ParamIDs::reverse) > 0.5f ? 1.0f : 0.12f; return std::pair<float, float> { b, b }; };
    record.onClick = [this] { toggle (ParamIDs::recOn); };
    record.onRightClick = [this] { cycle (ParamIDs::recTarget, 1); };
    record.onWheel = [this] (int d) { cycle (ParamIDs::recTarget, d); };
    record.onLongPress = [this] { toggle (ParamIDs::halfSpeed); };
    record.ledState = [this] {
        float b = value (ParamIDs::recOn) > 0.5f ? 1.0f : 0.18f;
        if (value (ParamIDs::halfSpeed) > 0.5f)
            b *= (Time::getMillisecondCounter() % 1000) < 500 ? 1.0f : 0.25f;
        const int t = (int) std::lround (value (ParamIDs::recTarget) * 2.0f);
        return std::pair<float, float> { t != 1 ? b : 0.0f, t != 0 ? b : 0.0f };
    };

    chipMono.onClick = [this] { toggle (ParamIDs::inputMode); };
    chipMono.isOn = [this] { return value (ParamIDs::inputMode) > 0.5f; };
    chipGate.onClick = [this] { toggle (ParamIDs::gate); };
    chipGate.isOn = [this] { return value (ParamIDs::gate) > 0.5f; };
    chipHalf.onClick = [this] { toggle (ParamIDs::halfSpeed); };
    chipHalf.isOn = [this] { return value (ParamIDs::halfSpeed) > 0.5f; };
    chipSave.onClick = [this] { toggle (ParamIDs::saveMemory); };
    chipSave.isOn = [this] { return value (ParamIDs::saveMemory) > 0.5f; };
    chipEraseAll.onClick = [this] { proc.triggerEraseAll(); };
    chipEraseAll.isOn = [] { return false; };
    chipBeads.onClick = [this] { toggle (ParamIDs::beadsOn); };
    chipBeads.isOn = [this] { return value (ParamIDs::beadsOn) > 0.5f; };

    setSize (W, H);
    buildBackground();
}

float Panel::value (const char* id) const
{
    if (auto* p = proc.apvts.getParameter (id)) return p->getValue();
    return 0.0f;
}

void Panel::setParam (const char* id, float normalised)
{
    if (auto* p = proc.apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (normalised);
        p->endChangeGesture();
    }
}

void Panel::toggle (const char* id) { setParam (id, value (id) > 0.5f ? 0.0f : 1.0f); }

void Panel::cycle (const char* id, int delta)
{
    if (auto* c = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter (id)))
    {
        const int n = c->choices.size();
        const int idx = ((c->getIndex() + delta) % n + n) % n;
        setParam (id, c->convertTo0to1 ((float) idx));
    }
}

void Panel::buildBackground()
{
    const int scale = 2;
    background = Image (Image::ARGB, W * scale, H * scale, true);
    Graphics g (background);
    g.addTransform (AffineTransform::scale ((float) scale));

    const auto r = Rectangle<float> (0, 0, (float) W, (float) H);
    ColourGradient body (Theme::bgTop, 0, 0, Theme::bgBottom, 0, (float) H, false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (r, 16.0f);

    // powder-coat grain
    Random rnd (20211103);
    for (int i = 0; i < 70000; ++i)
    {
        const float x = rnd.nextFloat() * (float) W, y = rnd.nextFloat() * (float) H;
        g.setColour ((rnd.nextBool() ? Colours::white : Colours::black).withAlpha (0.025f + 0.04f * rnd.nextFloat()));
        g.fillRect (x, y, 0.6f, 0.6f);
    }
    // soft glow behind the dial
    ColourGradient glow (Theme::accent.withAlpha (0.10f), kDialCentre.x, kDialCentre.y, Theme::accent.withAlpha (0.0f), kDialCentre.x + 210.0f, kDialCentre.y, true);
    g.setGradientFill (glow);
    g.fillEllipse (kDialCentre.x - 210.0f, kDialCentre.y - 210.0f, 420.0f, 420.0f);
    // vignette
    ColourGradient vig (Colours::transparentBlack, (float) W * 0.5f, (float) H * 0.45f, Colours::black.withAlpha (0.35f), 0.0f, 0.0f, true);
    g.setGradientFill (vig);
    g.fillRoundedRectangle (r, 16.0f);
    // edges
    g.setColour (Colours::white.withAlpha (0.08f));
    g.drawLine (18.0f, 1.0f, (float) W - 18.0f, 1.0f, 1.0f);
    g.setColour (Colours::white.withAlpha (0.05f));
    g.drawRoundedRectangle (r.reduced (0.5f), 16.0f, 1.0f);

    // corner screws
    for (auto p : { Point<float> (16, 16), Point<float> ((float) W - 16, 16), Point<float> (16, (float) H - 16), Point<float> ((float) W - 16, (float) H - 16) })
    {
        ColourGradient sg (Colour (0xff4a4a50), p.x - 4, p.y - 4, Colour (0xff141416), p.x + 4, p.y + 4, false);
        g.setGradientFill (sg);
        g.fillEllipse (p.x - 4.5f, p.y - 4.5f, 9.0f, 9.0f);
        g.setColour (Colours::black.withAlpha (0.7f));
        g.drawLine (p.x - 2.6f, p.y - 2.6f, p.x + 2.6f, p.y + 2.6f, 1.2f);
        g.drawLine (p.x - 2.6f, p.y + 2.6f, p.x + 2.6f, p.y - 2.6f, 1.2f);
    }

    // header bar: logo on the left, chips on the right (placed as components)
    g.setColour (Colours::black.withAlpha (0.25f));
    g.fillRect (Rectangle<float> (1.0f, 1.0f, (float) W - 2.0f, kHeader - 1.0f));
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawLine (18.0f, kHeader, (float) W - 18.0f, kHeader, 1.0f);
    g.setColour (Theme::text);
    const auto logoFont = Fonts::bold (28.0f, 0.20f);
    g.setFont (logoFont);
    g.drawText ("DERIVA", Rectangle<float> (34, 8, 220, 34), Justification::centredLeft);
    const float logoW = GlyphArrangement::getStringWidth (logoFont, "DERIVA");
    g.setColour (Theme::dim);
    g.setFont (Fonts::semibold (9.5f, 0.32f));
    g.drawText ("PRIME MEMORY DRIFTER", Rectangle<float> (34 + logoW + 16, 19, 240, 14), Justification::centredLeft);
    // little orbit mark next to the algorithm caption
    g.setColour (Theme::accent);
    g.drawEllipse (36.0f, kHeader + 26.0f, 10.0f, 10.0f, 1.2f);
    g.fillEllipse (44.0f, kHeader + 24.0f, 4.0f, 4.0f);
}

void Panel::refresh()
{
    dial.updateFromEngine();
    if (proc.isEnginePrepared())
    {
        const auto& e = proc.getEngine();
        const float pk = e.lastLoopPeak();
        meterLevel = jmax (pk, meterLevel * 0.82f);
        if (e.limiterWasActive()) limHold = 10; else if (limHold > 0) --limHold;
        if (e.inputClipped()) clipHold = 15; else if (clipHold > 0) --clipHold;
        inLevel = jmax (e.lastInputPeak(), inLevel * 0.8f);
    }
    limiterOn = limHold > 0;
    inClip = clipHold > 0;
    for (auto* f : { &hpf, &lpf, &erase, &reverse, &record }) f->tick();
    for (auto* c : { &chipMono, &chipGate, &chipHalf, &chipSave, &chipBeads }) c->repaint();
    repaint (Rectangle<int> ((int) kDialCentre.x - 60, (int) kMeterY - 10, 120, 22));
    repaint (Rectangle<int> (20, (int) kIn.cy - 46, 120, 40));
    repaint (Rectangle<int> (50, (int) kHeader + 20, 330, 22));
}

void Panel::paint (Graphics& g)
{
    g.drawImage (background, getLocalBounds().toFloat());

    // algorithm caption
    {
        const int a = (int) std::lround (proc.apvts.getRawParameterValue (ParamIDs::algo)->load());
        auto* p = proc.apvts.getParameter (ParamIDs::algo);
        g.setColour (Theme::dim);
        g.setFont (Fonts::semibold (8.5f, 0.25f));
        String cap = p != nullptr ? p->getCurrentValueAsText() : String (a);
        if (proc.isEnginePrepared())
        {
            const auto& e = proc.getEngine();
            cap << "   " << String (e.numLinesNow()) << " LINES   " << String (e.lineLengthSeconds (0), 1) << " S";
        }
        g.drawText (cap, Rectangle<float> (54, kHeader + 24.0f, 320, 14), Justification::centredLeft);
    }

    // loop level meter (5 LEDs)
    {
        const float lvl = Decibels::gainToDecibels (meterLevel, -100.0f);
        const float th[4] = { -36.0f, -24.0f, -12.0f, -6.0f };
        const Colour cols[5] = { Theme::ledGreen, Theme::ledGreen, Theme::ledYellow, Theme::ledYellow, Theme::ledRed };
        for (int i = 0; i < 5; ++i)
        {
            const float b = i < 4 ? (lvl > th[i] ? 1.0f : 0.0f) : (limiterOn ? 1.0f : 0.0f);
            drawLed (g, { kDialCentre.x + ((float) i - 2.0f) * 14.0f, kMeterY }, 3.2f, cols[i], b);
        }
    }

    // IN led (green on signal, red on A/D overload) and OUT power led
    drawLed (g, { kIn.cx + 34.0f, kIn.cy - 30.0f }, 3.2f, inClip ? Theme::ledRed : Theme::ledGreen,
             inClip ? 1.0f : jlimit (0.0f, 1.0f, inLevel * 6.0f));
    drawLed (g, { kOut.cx - 34.0f, kOut.cy - 30.0f }, 3.2f, Theme::ledGreen, 0.9f);
}

//==============================================================================
ChoiceRow::ChoiceRow (RangedAudioParameter& p, const StringArray& l, const String& tip)
    : param (p), attachment (p, [this] (float v) { index = jmax (0, (int) std::lround (v)); repaint(); }, nullptr), labels (l)
{
    setTooltip (tip);
    attachment.sendInitialUpdate();
}

void ChoiceRow::paint (Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (Theme::text.withAlpha (0.16f));
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
    const float segW = r.getWidth() / (float) labels.size();
    for (int i = 0; i < labels.size(); ++i)
    {
        auto seg = Rectangle<float> (r.getX() + segW * (float) i, r.getY(), segW, r.getHeight());
        const bool on = i == index;
        if (on)
        {
            g.setColour (Theme::accent.withAlpha (0.22f));
            g.fillRoundedRectangle (seg.reduced (2.0f), (seg.getHeight() - 4.0f) * 0.5f);
        }
        if (i > 0)
        {
            g.setColour (Theme::text.withAlpha (0.10f));
            g.drawLine (seg.getX(), seg.getY() + 5.0f, seg.getX(), seg.getBottom() - 5.0f, 1.0f);
        }
        g.setFont (Fonts::semibold (8.8f, 0.14f));
        const float tw = GlyphArrangement::getStringWidth (g.getCurrentFont(), labels[i]);
        const float x0 = seg.getCentreX() - (tw + 11.0f) * 0.5f;
        drawLed (g, { x0 + 3.0f, seg.getCentreY() }, 2.6f, Theme::accentLight, on ? 1.0f : 0.0f);
        g.setColour (on ? Theme::text : Theme::dim);
        g.drawText (labels[i], Rectangle<float> (x0 + 10.0f, seg.getY(), tw + 4.0f, seg.getHeight()), Justification::centredLeft);
    }
}

void ChoiceRow::mouseDown (const MouseEvent& e)
{
    const int i = jlimit (0, labels.size() - 1, (int) (e.position.x / ((float) getWidth() / (float) labels.size())));
    attachment.setValueAsCompleteGesture ((float) i);
}

//==============================================================================
namespace
{
    constexpr float kBRowY = 112.0f, kBRndY = 208.0f;
    constexpr float kBX[8] = { 320, 410, 500, 590, 700, 790, 880, 970 };
}

BeadsPanel::BeadsPanel (DerivaProcessor& p)
    : proc (p),
      time (p.apvts, ParamIDs::bTime, "TIME", Knob::Kind::Normal, "TIME: de dónde se leen los granos en el buffer de 4 s (de lo más reciente a lo más antiguo)."),
      size (p.apvts, ParamIDs::bSize, "SIZE", Knob::Kind::Normal, "SIZE: duración del grano. Por debajo de 1/3 los granos suenan al revés; al máximo, Beads pasa a modo delay."),
      shape (p.apvts, ParamIDs::bShape, "SHAPE", Knob::Kind::Normal, "SHAPE: envolvente del grano, de percusiva a suave e invertida."),
      pitch (p.apvts, ParamIDs::bPitch, "PITCH", Knob::Kind::Bipolar, "PITCH: transposición de los granos, ±24 semitonos con paradas en 1, 3, 4, 7, 12 y 24."),
      density (p.apvts, ParamIDs::bDensity, "DENSITY", Knob::Kind::Bipolar, "DENSITY: en el centro no hay granos (solo SEED). A la izquierda, granos aleatorios; a la derecha, regulares. Con SYNC: divisor / probabilidad del reloj."),
      feedback (p.apvts, ParamIDs::bFeedback, "FEEDBACK", Knob::Kind::Normal, "FEEDBACK de Beads: reinyecta los granos en su propio buffer. Se anula en modo LOOP."),
      drywet (p.apvts, ParamIDs::bDryWet, "DRY / WET", Knob::Kind::Normal, "WET: mezcla de la memoria de Deriva con los granos. LOOP: cuánto de la realimentación de Deriva pasa por Beads."),
      reverb (p.apvts, ParamIDs::bReverb, "REVERB", Knob::Kind::Normal, "Reverb de Beads (Griesinger / Dattorro). Se anula en modo LOOP."),
      timeRnd (p.apvts, ParamIDs::bTimeRnd, "RANDOM", Knob::Kind::Mini, "Aleatoriedad de TIME: a la derecha, variación aleatoria por grano; a la izquierda, deriva suave."),
      sizeRnd (p.apvts, ParamIDs::bSizeRnd, "RANDOM", Knob::Kind::Mini, "Aleatoriedad de SIZE."),
      shapeRnd (p.apvts, ParamIDs::bShapeRnd, "RANDOM", Knob::Kind::Mini, "Aleatoriedad de SHAPE."),
      pitchRnd (p.apvts, ParamIDs::bPitchRnd, "RANDOM", Knob::Kind::Mini, "Aleatoriedad de PITCH (hasta ±24 semitonos)."),
      freeze ("FREEZE", "", FootSwitch::LedMode::Single, "Congela el buffer de Beads: deja de grabar y los granos leen siempre lo mismo."),
      seed ("SEED", "HOLD 1 S: LATCH", FootSwitch::LedMode::Single, "SEED enclavado (LED latiendo): granos continuos según DENSITY. Pulsar: modo manual, cada pulsación dispara granos. Mantener 1 s: vuelve a enclavar."),
      quality (*p.apvts.getParameter (ParamIDs::bQuality), StringArray { "DIRTY TAPE", "CLEAN TAPE", "VINTAGE", "DIGITAL" },
               "Calidad del buffer: cinta sucia (8 bits µ-law, 24 kHz, wow y flutter), cinta limpia (12 bits, 24 kHz), digital vintage (12 bits, 32 kHz) o digital (16 bits, 48 kHz)."),
      place (*p.apvts.getParameter (ParamIDs::bPlace), StringArray { "WET", "LOOP" },
             "WET: Beads procesa la salida de la memoria antes de MIX. LOOP: la realimentación de Deriva pasa por Beads y cada repetición se vuelve a granular."),
      chipMono ("MONO", "Buffer mono de Beads (el doble de tiempo)."),
      chipSync ("SYNC", "Reloj de negras del host en la entrada SEED: con SEED enclavado, DENSITY elige divisor y probabilidad."),
      chipClose ("CLOSE", "Cierra el panel y desactiva Beads.")
{
    Knob* mains[8] = { &time, &size, &shape, &pitch, &density, &feedback, &drywet, &reverb };
    for (int i = 0; i < 8; ++i)
    {
        addAndMakeVisible (mains[i]);
        const int S = (int) (mains[i]->knobRadius() * 2.0f + 64.0f);
        mains[i]->setBounds ((int) kBX[i] - S / 2, (int) kBRowY - S / 2, S, S);
    }
    Knob* rnds[4] = { &timeRnd, &sizeRnd, &shapeRnd, &pitchRnd };
    for (int i = 0; i < 4; ++i)
    {
        addAndMakeVisible (rnds[i]);
        const int S = (int) (rnds[i]->knobRadius() * 2.0f + 64.0f);
        rnds[i]->setBounds ((int) kBX[i] - S / 2, (int) kBRndY - S / 2, S, S);
    }
    addAndMakeVisible (freeze);
    addAndMakeVisible (seed);
    freeze.setBounds (40, 118, 110, 130);
    seed.setBounds (150, 118, 110, 130);
    addAndMakeVisible (quality);
    quality.setBounds (640, 200, 366, 26);
    addAndMakeVisible (place);
    place.setBounds (640, 250, 140, 26);
    for (auto* c : { &chipMono, &chipSync, &chipClose }) addAndMakeVisible (c);
    chipMono.setBounds (792, 252, chipMono.idealWidth(), 22);
    chipSync.setBounds (chipMono.getRight() + 8, 252, chipSync.idealWidth(), 22);
    chipClose.setBounds (1006 - chipClose.idealWidth(), 252, chipClose.idealWidth(), 22);

    freeze.onClick = [this] { setParam (ParamIDs::bFreeze, value (ParamIDs::bFreeze) > 0.5f ? 0.0f : 1.0f); };
    freeze.ledState = [this] { const float b = value (ParamIDs::bFreeze) > 0.5f ? 1.0f : 0.12f; return std::pair<float, float> { b, b }; };
    freeze.ledColour = Theme::ledWhite;
    seed.longPressMs = 1000;
    seed.onPress = [this] { if (value (ParamIDs::bSeedLock) > 0.5f) setParam (ParamIDs::bSeedLock, 0.0f); setParam (ParamIDs::bSeed, 1.0f); };
    seed.onRelease = [this] { setParam (ParamIDs::bSeed, 0.0f); };
    seed.onLongPress = [this] { setParam (ParamIDs::bSeedLock, 1.0f); };
    seed.ledColour = Theme::accentLight;
    seed.ledState = [this] {
        float b = 0.12f;
        if (value (ParamIDs::bSeed) > 0.5f) b = 1.0f;
        else if (value (ParamIDs::bSeedLock) > 0.5f)
        {
            const float t = (float) (Time::getMillisecondCounter() % 1600) / 1600.0f;
            b = 0.3f + 0.6f * (t < 0.5f ? t * 2.0f : 2.0f - t * 2.0f);
        }
        return std::pair<float, float> { b, b };
    };
    chipMono.onClick = [this] { setParam (ParamIDs::bMono, value (ParamIDs::bMono) > 0.5f ? 0.0f : 1.0f); };
    chipMono.isOn = [this] { return value (ParamIDs::bMono) > 0.5f; };
    chipSync.onClick = [this] { setParam (ParamIDs::bSync, value (ParamIDs::bSync) > 0.5f ? 0.0f : 1.0f); };
    chipSync.isOn = [this] { return value (ParamIDs::bSync) > 0.5f; };
    chipClose.onClick = [this] { setParam (ParamIDs::beadsOn, 0.0f); };
    chipClose.isOn = [] { return false; };

    setSize (W, H);
    buildBackground();
}

float BeadsPanel::value (const char* id) const
{
    if (auto* p = proc.apvts.getParameter (id)) return p->getValue();
    return 0.0f;
}

void BeadsPanel::setParam (const char* id, float normalised)
{
    if (auto* p = proc.apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (normalised);
        p->endChangeGesture();
    }
}

void BeadsPanel::buildBackground()
{
    const int scale = 2;
    background = Image (Image::ARGB, W * scale, H * scale, true);
    Graphics g (background);
    g.addTransform (AffineTransform::scale ((float) scale));
    const auto r = Rectangle<float> (0, 0, (float) W, (float) H);
    ColourGradient body (Colour (0xff141417), 0, 0, Colour (0xff0a0a0c), 0, (float) H, false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (r, 16.0f);
    Random rnd (4040);
    for (int i = 0; i < 45000; ++i)
    {
        const float x = rnd.nextFloat() * (float) W, y = rnd.nextFloat() * (float) H;
        g.setColour ((rnd.nextBool() ? Colours::white : Colours::black).withAlpha (0.025f + 0.04f * rnd.nextFloat()));
        g.fillRect (x, y, 0.6f, 0.6f);
    }
    // hinge with the main panel
    g.setColour (Colours::black);
    g.fillRect (0.0f, 0.0f, (float) W, 5.0f);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawLine (16.0f, 6.0f, (float) W - 16.0f, 6.0f, 1.0f);
    g.setColour (Colours::white.withAlpha (0.05f));
    g.drawRoundedRectangle (r.reduced (0.5f), 16.0f, 1.0f);
    for (auto p : { Point<float> (16, (float) H - 16), Point<float> ((float) W - 16, (float) H - 16) })
    {
        ColourGradient sg (Colour (0xff4a4a50), p.x - 4, p.y - 4, Colour (0xff141416), p.x + 4, p.y + 4, false);
        g.setGradientFill (sg);
        g.fillEllipse (p.x - 4.5f, p.y - 4.5f, 9.0f, 9.0f);
        g.setColour (Colours::black.withAlpha (0.7f));
        g.drawLine (p.x - 2.6f, p.y - 2.6f, p.x + 2.6f, p.y + 2.6f, 1.2f);
        g.drawLine (p.x - 2.6f, p.y + 2.6f, p.x + 2.6f, p.y - 2.6f, 1.2f);
    }

    g.setColour (Theme::text);
    g.setFont (Fonts::bold (30.0f, 0.24f));
    g.drawText ("BEADS", Rectangle<float> (34, 22, 260, 36), Justification::centredLeft);
    g.setColour (Theme::dim);
    g.setFont (Fonts::semibold (9.0f, 0.3f));
    g.drawText ("GRANULAR TEXTURE STAGE", Rectangle<float> (36, 58, 300, 12), Justification::centredLeft);

    // section headers
    auto header = [&g] (const String& t, float x0, float x1, float y)
    {
        g.setFont (Fonts::semibold (8.5f, 0.32f));
        const float tw = GlyphArrangement::getStringWidth (g.getCurrentFont(), t);
        const float cx = 0.5f * (x0 + x1);
        g.setColour (Theme::dim);
        g.drawText (t, Rectangle<float> (cx - tw * 0.5f - 4, y - 6, tw + 8, 12), Justification::centred);
        g.setColour (Theme::text.withAlpha (0.12f));
        g.drawLine (x0, y, cx - tw * 0.5f - 10, y, 1.0f);
        g.drawLine (cx + tw * 0.5f + 10, y, x1, y, 1.0f);
    };
    header ("GRAIN", 268, 642, 34);
    header ("OUTPUT", 648, 1022, 34);
    g.setColour (Theme::dim);
    g.setFont (Fonts::semibold (8.5f, 0.32f));
    g.drawText ("QUALITY", Rectangle<float> (640, 184, 200, 12), Justification::centredLeft);

    g.setColour (Theme::dim.withAlpha (0.7f));
    g.setFont (Fonts::medium (7.5f, 0.18f));
    g.drawText ("DSP: EMILIE GILLET / MUTABLE INSTRUMENTS (MIT)", Rectangle<float> (36, 270, 400, 12), Justification::centredLeft);
}

void BeadsPanel::refresh()
{
    activity = jmax (proc.beadsActivity(), activity * 0.85f);
    freeze.tick();
    seed.tick();
    for (auto* c : { &chipMono, &chipSync }) c->repaint();
    repaint (Rectangle<int> (30, 80, 240, 24));
}

void BeadsPanel::paint (Graphics& g)
{
    g.drawImage (background, getLocalBounds().toFloat());
    // a string of beads that lights up with the stage output
    const float lvl = Decibels::gainToDecibels (activity, -100.0f);
    Path string;
    for (int i = 0; i < 10; ++i)
    {
        const float x = 40.0f + (float) i * 22.0f, y = 92.0f + 3.0f * std::sin ((float) i * 0.7f);
        if (i == 0) string.startNewSubPath (x, y); else string.lineTo (x, y);
    }
    g.setColour (Theme::text.withAlpha (0.12f));
    g.strokePath (string, PathStrokeType (1.0f));
    for (int i = 0; i < 10; ++i)
    {
        const float x = 40.0f + (float) i * 22.0f, y = 92.0f + 3.0f * std::sin ((float) i * 0.7f);
        const float th = -54.0f + (float) i * 5.0f;
        const float b = jlimit (0.0f, 1.0f, (lvl - th) / 5.0f);
        drawLed (g, { x, y }, 4.2f, Theme::accentLight, b);
    }
}

} // namespace ui

//==============================================================================
DerivaEditor::DerivaEditor (DerivaProcessor& p)
    : AudioProcessorEditor (p), proc (p), panel (p), beadsPanel (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (stack);
    stack.addAndMakeVisible (panel);
    stack.addChildComponent (beadsPanel);
    panel.setTopLeftPosition (0, 0);
    beadsPanel.setTopLeftPosition (0, ui::Panel::H);
    tooltips.setLookAndFeel (&lnf);
    setResizable (true, true);
    beadsOpen = proc.apvts.getRawParameterValue (ParamIDs::beadsOn)->load() > 0.5f;
    setSize (roundToInt (ui::Panel::W * 1.1f), roundToInt (ui::Panel::H * 1.1f));   // default: 10 % larger
    layoutForState (beadsOpen);
    startTimerHz (30);
}

void DerivaEditor::layoutForState (bool open)
{
    beadsPanel.setVisible (open);
    const int baseH = ui::Panel::H + (open ? ui::BeadsPanel::H : 0);
    const float s = jmax (0.5f, (float) getWidth() / (float) ui::Panel::W);
    const double aspect = (double) ui::Panel::W / (double) baseH;
    getConstrainer()->setFixedAspectRatio (0.0);
    setResizeLimits (780, (int) std::lround (780.0 / aspect), 1820, (int) std::lround (1820.0 / aspect));
    setSize (roundToInt ((float) ui::Panel::W * s), roundToInt ((float) baseH * s));
    getConstrainer()->setFixedAspectRatio (aspect);
    resized();
}

DerivaEditor::~DerivaEditor()
{
    stopTimer();
    tooltips.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void DerivaEditor::resized()
{
    const float s = (float) getWidth() / (float) ui::Panel::W;
    stack.setBounds (0, 0, ui::Panel::W, ui::Panel::H + (beadsOpen ? ui::BeadsPanel::H : 0));
    stack.setTransform (AffineTransform::scale (s));
}

void DerivaEditor::timerCallback()
{
    const bool want = proc.apvts.getRawParameterValue (ParamIDs::beadsOn)->load() > 0.5f;
    if (want != beadsOpen)
    {
        beadsOpen = want;
        layoutForState (beadsOpen);
    }
    panel.refresh();
    if (beadsOpen) beadsPanel.refresh();
}
