#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "SwellLookAndFeel.h"

using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

struct ControlSpec
{
    const char* name;
    const char* parameter;
};
struct EffectSpec
{
    const char* name;
    const char* power;
    const char* mode;
    std::array<ControlSpec, 4> controls;
};

class ParameterKnob : public juce::Component
{
  public:
    explicit ParameterKnob (SwellAdLibProcessor&);
    void bind (const ControlSpec&, juce::Colour, std::function<void()> enable = {});
    void paint (juce::Graphics&) override;
    void resized() override;

  private:
    SwellAdLibProcessor& proc;
    juce::Slider dial;
    juce::String label;
    juce::Colour ink;
    std::unique_ptr<SliderAttachment> attachment;
};

// Four signal stages have independent on/off, algorithm choice and wet amount.
class EffectStrip : public juce::Component
{
  public:
    EffectStrip (SwellAdLibProcessor&, int effect);
    void refresh();
    void paint (juce::Graphics&) override;
    void resized() override;

  private:
    SwellAdLibProcessor& proc;
    int index;
    juce::TextButton power;
    juce::ComboBox mode;
    ParameterKnob amount;
    bool active = false;
};

class DetailView : public juce::Component
{
  public:
    explicit DetailView (SwellAdLibProcessor&);
    void refresh();
    void paint (juce::Graphics&) override;
    void resized() override;

  private:
    void select (int);
    SwellAdLibProcessor& proc;
    juce::ComboBox group, mode;
    juce::TextButton power;
    juce::OwnedArray<ParameterKnob> knobs;
    int selected = 0;
};

class SwellAdLibEditor : public juce::AudioProcessorEditor, private juce::Timer
{
  public:
    explicit SwellAdLibEditor (SwellAdLibProcessor&);
    ~SwellAdLibEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

  private:
    void timerCallback() override;
    void refresh();
    void showDetails (bool);
    void zoomMenu();

    SwellAdLibProcessor& proc;
    SwellLookAndFeel lnf;
    juce::ComboBox presets;
    juce::TextButton previous{"<"}, next{">"}, compare{"A / B"}, bypass{"Bypass"};
    juce::OwnedArray<EffectStrip> strips;
    DetailView detail;
    juce::TextButton detailsButton, throwButton{"Throw"}, zoom;
    juce::Slider mix, gain;
    std::unique_ptr<SliderAttachment> mixAttachment, gainAttachment;
    juce::TooltipWindow tooltips{this, 450};
    bool detailsShown = false;
    int shownPreset = -1;
    float levels[2]{};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SwellAdLibEditor)
};
