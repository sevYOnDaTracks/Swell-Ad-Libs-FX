#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

// Shared Swell brand style: anthracite panels, orange accents
namespace SwellColours
{
inline const juce::Colour background { 0xff10151b };
inline const juce::Colour panel      { 0xff1c232c };
inline const juce::Colour panelDark  { 0xff1a1a1d };
inline const juce::Colour panelLight { 0xff273240 };
inline const juce::Colour well       { 0xff11171d };
inline const juce::Colour control    { 0xff26313e };
inline const juce::Colour border     { 0xff354353 };
inline const juce::Colour borderHi   { 0xff47566a };
inline const juce::Colour track      { 0xff354353 };
inline const juce::Colour orange     { 0xffedac75 };
inline const juce::Colour orangeDeep { 0xff5a3a22 };
inline const juce::Colour text       { 0xffececf0 };
inline const juce::Colour dim        { 0xffa9b5c4 };
inline const juce::Colour accentBlue { 0xff99b9de };
} // namespace SwellColours

inline juce::Font swellFont (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

// The Swell logo (white on transparent, from Resources/swell_logo.png)
inline void drawSwellLogo (juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto logo = juce::ImageCache::getFromMemory (BinaryData::swell_logo_png, BinaryData::swell_logo_pngSize);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (logo, r, juce::RectanglePlacement::centred);
}

class SwellLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SwellLookAndFeel()
    {
        using namespace SwellColours;
        setColour (juce::ResizableWindow::backgroundColourId, background);
        setColour (juce::ComboBox::backgroundColourId, well);
        setColour (juce::ComboBox::textColourId, text);
        setColour (juce::ComboBox::outlineColourId, borderHi);
        setColour (juce::ComboBox::arrowColourId, dim);
        setColour (juce::Slider::rotarySliderFillColourId, accentBlue);
        setColour (juce::PopupMenu::backgroundColourId, panelLight);
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::headerTextColourId, dim);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, orange);
        setColour (juce::PopupMenu::highlightedTextColourId, background);
        setColour (juce::TextButton::buttonColourId, control);
        setColour (juce::TextButton::buttonOnColourId, orange);
        setColour (juce::TextButton::textColourOffId, text);
        setColour (juce::TextButton::textColourOnId, text);
        setColour (juce::Slider::textBoxTextColourId, text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, dim);
    }

    // Big macro knob: orange value ring around a dark domed knob with an orange pointer
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                           float startAngle, float endAngle, juce::Slider& slider) override
    {
        using namespace SwellColours;
        const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (2.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float ringW = radius * 0.19f;
        const float arcR = radius - ringW * 0.5f;
        const float angle = startAngle + pos * (endAngle - startAngle);
        const juce::PathStrokeType stroke (ringW, juce::PathStrokeType::curved, juce::PathStrokeType::butt);

        juce::Path bg;
        bg.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (track);
        g.strokePath (bg, stroke);

        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId).withMultipliedAlpha (slider.isEnabled() ? 1.0f : 0.4f));
        g.strokePath (value, stroke);

        const float knobR = radius * 0.67f;
        const auto knob = juce::Rectangle<float> (knobR * 2.0f, knobR * 2.0f).withCentre (centre);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillEllipse (knob.translated (0.0f, 4.0f).expanded (1.0f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a41), centre.x - knobR * 0.3f, centre.y - knobR * 0.45f,
                                                 juce::Colour (0xff1c1c20), centre.x + knobR * 0.4f, centre.y + knobR * 0.7f, true));
        g.fillEllipse (knob);

        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
        g.drawLine ({ centre.getPointOnCircumference (knobR * 0.45f, angle), centre.getPointOnCircumference (knobR * 0.85f, angle) }, 4.0f);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float, float, juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        using namespace SwellColours;
        const float cy = (float) y + (float) height * 0.5f;
        const auto trackRect = juce::Rectangle<float> ((float) x, cy - 2.0f, (float) width, 4.0f);
        g.setColour (track);
        g.fillRoundedRectangle (trackRect, 2.0f);
        g.setColour (slider.isEnabled() ? orange : orange.withAlpha (0.3f));
        g.fillRoundedRectangle (trackRect.withRight (sliderPos), 2.0f);
        g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ sliderPos, cy }));
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> ((float) width, (float) height).reduced (0.5f);
        g.setColour (box.findColour (juce::ComboBox::backgroundColourId)); g.fillRoundedRectangle (r, 5);
        g.setColour (box.findColour (juce::ComboBox::outlineColourId)); g.drawRoundedRectangle (r, 5, 1);
        const float x = r.getRight() - 14, y = r.getCentreY();
        juce::Path arrow;
        arrow.startNewSubPath (x-3, y-1.5f); arrow.lineTo (x,y+1.5f); arrow.lineTo (x+3,y-1.5f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId)); g.strokePath (arrow,juce::PathStrokeType (1.4f));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return swellFont (14.0f, true); }
    juce::Font getPopupMenuFont() override { return swellFont (14.0f); }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&, bool over, bool down) override
    {
        using namespace SwellColours;
        const auto r = button.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = button.getToggleState();
        const float corner = (float) button.getProperties().getWithDefault ("corner", 8.0f);

        auto colour = on ? juce::Colour (0xff354c62) : control;
        if (over) colour = colour.brighter (0.08f);
        if (down) colour = colour.darker (0.1f);

        g.setColour (colour);
        g.fillRoundedRectangle (r, corner);
        g.setColour (on ? accentBlue.withAlpha (0.65f) : borderHi);
        g.drawRoundedRectangle (r, corner, 1.0f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override { return swellFont (13.0f, true); }
};
