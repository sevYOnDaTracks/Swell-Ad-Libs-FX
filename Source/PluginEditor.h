#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "SwellLookAndFeel.h"

using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

// Scrolling level of the output signal (the "LIVE" strip)
class LiveDisplay : public juce::Component
{
public:
    explicit LiveDisplay (SwellAdLibProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;

private:
    SwellAdLibProcessor& proc;
};

// The four big knobs: SPACE, DIRT, PITCH, WIDTH
class MacroPanel : public juce::Component
{
public:
    explicit MacroPanel (SwellAdLibProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Rectangle<int> card (int i) const;
    void ensureModuleOn (int macro);

    SwellAdLibProcessor& proc;
    juce::Slider knobs[4];
    std::unique_ptr<SliderAttachment> attachments[4];
};

// One effect of the chain: icon + name, orange when on, outlined when selected
class ModuleButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    ModuleButton (const juce::String& name, const juce::String& svgIcon);
    void setState (bool isOn, bool isSelected);
    std::function<void()> onClick;

    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

private:
    juce::String name;
    juce::Path icon;
    bool on = false, selected = false;
};

// Settings of the selected effect: ON/OFF, mode buttons and three sliders
class ModulePanel : public juce::Component
{
public:
    explicit ModulePanel (SwellAdLibProcessor&);
    void setModule (int index);
    void refresh();
    std::function<void()> onBack;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void pickMode (int k);

    SwellAdLibProcessor& proc;
    int module = 0;
    juce::TextButton back { "< Macros" }, power { "ON" };
    juce::OwnedArray<juce::TextButton> modeButtons;
    juce::Slider sliders[3];
    std::unique_ptr<SliderAttachment> sliderAtt[3];
    std::unique_ptr<ButtonAttachment> powerAtt;
};

// Presets of the chosen category
class PresetList : public juce::Component
{
public:
    explicit PresetList (SwellAdLibProcessor& p) : proc (p) {}
    void setCategory (const juce::String&);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }

private:
    SwellAdLibProcessor& proc;
    std::vector<int> items;
    int hover = -1;
    static constexpr int rowH = 44;
};

// Current preset in the header; click for the full menu
class PresetDisplay : public juce::Component
{
public:
    explicit PresetDisplay (SwellAdLibProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    std::function<void (int)> onPick;

private:
    SwellAdLibProcessor& proc;
};

class ThrowButton : public juce::Button
{
public:
    ThrowButton() : juce::Button ("Throw") { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

class Meter : public juce::Component
{
public:
    void setLevel (float l) { level = l; repaint(); }
    void paint (juce::Graphics&) override;

private:
    float level = 0.0f;
};

//==============================================================================
class SwellAdLibEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SwellAdLibEditor (SwellAdLibProcessor&);
    ~SwellAdLibEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void selectModule (int index);
    void selectCategory (const juce::String&);
    void presetChanged();

    SwellAdLibProcessor& proc;
    SwellLookAndFeel lnf;

    juce::TextButton prevButton { "<" }, nextButton { ">" }, abButton { "A" }, bypassButton { "BYPASS" };
    PresetDisplay presetDisplay;
    juce::OwnedArray<juce::TextButton> categoryButtons;
    PresetList presetList;
    juce::String category;

    LiveDisplay live;
    MacroPanel macros;
    ModulePanel modulePanel;
    juce::OwnedArray<ModuleButton> moduleButtons;
    int selected = -1;

    ThrowButton throwButton;
    Meter meterL, meterR;
    juce::Slider mixSlider, gainSlider;
    std::unique_ptr<SliderAttachment> mixAtt, gainAtt;
    std::unique_ptr<ButtonAttachment> throwAtt, bypassAtt;

    juce::TooltipWindow tooltips { this, 400 };
    int shownPreset = -1;
    float levelL = 0.0f, levelR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SwellAdLibEditor)
};
