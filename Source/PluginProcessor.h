#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "AdLibEngine.h"

class SwellAdLibProcessor : public juce::AudioProcessor
{
public:
    SwellAdLibProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Swell Ad-lib FX"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // Presets and A/B comparison (message thread)
    void loadPreset (int index);
    int getCurrentPreset() const { return currentPreset.load(); }
    void toggleAB();
    bool isOnB() const { return onB; }

    // Metering, read by the editor
    std::atomic<float> peakL { 0.0f }, peakR { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    AdLib::Settings readSettings() const;
    float raw (const char* id) const;

    AdLib::Engine engine;
    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float> mixSmooth, gainSmooth;

    std::atomic<int> currentPreset { 0 };
    juce::ValueTree slotA, slotB;
    bool onB = false;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SwellAdLibProcessor)
};
