#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

namespace
{
juce::String percent (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }

juce::String hertz (float v, int)
{
    return v >= 1000.0f ? juce::String (v / 1000.0f, 1) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
}

std::unique_ptr<juce::AudioParameterFloat> amount (const char* id, const char* name, float def)
{
    return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (0.0f, 1.0f), def,
                                                        juce::AudioParameterFloatAttributes().withStringFromValueFunction (percent));
}

std::unique_ptr<juce::AudioParameterFloat> ranged (const char* id, const char* name, juce::NormalisableRange<float> range, float def,
                                                   std::function<juce::String (float, int)> toText)
{
    return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, range, def,
                                                        juce::AudioParameterFloatAttributes().withStringFromValueFunction (std::move (toText)));
}

std::unique_ptr<juce::AudioParameterBool> toggle (const char* id, const char* name, bool def)
{
    return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def);
}

std::unique_ptr<juce::AudioParameterChoice> choice (const char* id, const char* name, juce::StringArray items, int def)
{
    return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def);
}

juce::NormalisableRange<float> skewed (float lo, float hi, float centre)
{
    juce::NormalisableRange<float> r (lo, hi);
    r.setSkewForCentre (centre);
    return r;
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout SwellAdLibProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Macros
    layout.add (amount ("space", "Space", 0.5f));
    layout.add (amount ("dirt", "Dirt", 0.3f));
    layout.add (ranged ("pitch", "Pitch", juce::NormalisableRange<float> (-12.0f, 12.0f, 1.0f), 0.0f,
                        [] (float v, int) { const int st = juce::roundToInt (v); return (st > 0 ? "+" : "") + juce::String (st) + " st"; }));
    layout.add (amount ("width", "Width", 0.5f));

    // 01 Clean
    layout.add (toggle ("clean_on", "Clean On", true));
    layout.add (choice ("clean_mode", "Clean Mode", { "High-pass", "Low-pass", "Band-pass" }, 0));
    layout.add (ranged ("clean_low", "Low Cut", skewed (20.0f, 600.0f, 150.0f), 150.0f, hertz));
    layout.add (ranged ("clean_high", "High Cut", skewed (2000.0f, 20000.0f, 8000.0f), 16000.0f, hertz));
    layout.add (amount ("clean_deess", "De-esser", 0.3f));

    // 02 Punch
    layout.add (toggle ("punch_on", "Punch On", true));
    layout.add (choice ("punch_mode", "Punch Mode", { "Smooth", "Punchy", "Smash" }, 1));
    layout.add (amount ("punch_amount", "Punch Amount", 0.5f));
    layout.add (ranged ("punch_attack", "Punch Attack", skewed (0.1f, 50.0f, 5.0f), 5.0f,
                        [] (float v, int) { return juce::String (v, v < 10.0f ? 1 : 0) + " ms"; }));
    layout.add (ranged ("punch_makeup", "Punch Volume", juce::NormalisableRange<float> (-12.0f, 12.0f), 0.0f,
                        [] (float v, int) { return (v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB"; }));

    // 03 Pitch (the semitones are the PITCH macro)
    layout.add (toggle ("pitch_on", "Pitch On", false));
    layout.add (amount ("pitch_mix", "Pitch Mix", 1.0f));
    layout.add (ranged ("pitch_fine", "Pitch Fine", juce::NormalisableRange<float> (-50.0f, 50.0f), 0.0f,
                        [] (float v, int) { return juce::String (juce::roundToInt (v)) + " ct"; }));

    // 04 Color (the drive is the DIRT macro)
    layout.add (toggle ("color_on", "Color On", false));
    layout.add (choice ("color_mode", "Color Mode", { "Tape", "Warm", "Fuzz", "Crush", "Phone", "Radio", "Megaphone" }, 0));
    layout.add (amount ("color_tone", "Color Tone", 0.6f));
    layout.add (amount ("color_mix", "Color Mix", 1.0f));

    // 05 Mod
    layout.add (toggle ("mod_on", "Mod On", false));
    layout.add (choice ("mod_mode", "Mod Mode", { "Doubler", "Chorus", "Wide" }, 0));
    layout.add (amount ("mod_depth", "Mod Depth", 0.5f));
    layout.add (ranged ("mod_rate", "Mod Rate", skewed (0.1f, 5.0f, 1.0f), 0.6f,
                        [] (float v, int) { return juce::String (v, 2) + " Hz"; }));
    layout.add (amount ("mod_mix", "Mod Mix", 0.5f));

    // 06 Delay
    layout.add (toggle ("delay_on", "Delay On", false));
    layout.add (choice ("delay_mode", "Delay Mode", { "1/4", "1/8", "1/16", "1/4 Dot", "Ping-pong" }, 0));
    layout.add (amount ("delay_feedback", "Delay Feedback", 0.35f));
    layout.add (amount ("delay_tone", "Delay Tone", 0.6f));
    layout.add (amount ("delay_mix", "Delay Mix", 0.35f));

    // 07 Reverb
    layout.add (toggle ("reverb_on", "Reverb On", false));
    layout.add (choice ("reverb_mode", "Reverb Mode", { "Room", "Plate", "Hall", "Dark", "Reverse" }, 1));
    layout.add (amount ("reverb_size", "Reverb Size", 0.5f));
    layout.add (ranged ("reverb_predelay", "Pre-delay", juce::NormalisableRange<float> (0.0f, 150.0f), 20.0f,
                        [] (float v, int) { return juce::String (juce::roundToInt (v)) + " ms"; }));
    layout.add (amount ("reverb_mix", "Reverb Mix", 0.35f));

    // Output
    layout.add (toggle ("throw", "Throw", false));
    layout.add (amount ("out_mix", "Mix", 1.0f));
    layout.add (ranged ("out_gain", "Gain", juce::NormalisableRange<float> (-12.0f, 12.0f), 0.0f,
                        [] (float v, int) { return (v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB"; }));
    layout.add (toggle ("bypass", "Bypass", false));

    return layout;
}

SwellAdLibProcessor::SwellAdLibProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SwellAdLibFX", createLayout())
{
    for (auto& w : wave) w.store (0.0f);
    loadPreset (0);
}

bool SwellAdLibProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    const auto in = layouts.getMainInputChannelSet();
    return out == juce::AudioChannelSet::stereo()
        && (in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono());
}

void SwellAdLibProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    dryBuffer.setSize (2, samplesPerBlock);
    mixSmooth.reset (sampleRate, 0.05);
    gainSmooth.reset (sampleRate, 0.05);
    mixSmooth.setCurrentAndTargetValue (raw ("out_mix"));
    gainSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (raw ("out_gain")));
}

float SwellAdLibProcessor::raw (const char* id) const
{
    return apvts.getRawParameterValue (id)->load();
}

AdLib::Settings SwellAdLibProcessor::readSettings() const
{
    AdLib::Settings s;
    auto on = [this] (const char* id) { return raw (id) > 0.5f; };
    auto mode = [this] (const char* id) { return juce::roundToInt (raw (id)); };

    s.cleanOn = on ("clean_on");   s.cleanMode = mode ("clean_mode");
    s.lowCut = raw ("clean_low");  s.highCut = raw ("clean_high");  s.deess = raw ("clean_deess");

    s.punchOn = on ("punch_on");   s.punchMode = mode ("punch_mode");
    s.punchAmount = raw ("punch_amount"); s.punchAttackMs = raw ("punch_attack"); s.punchMakeupDb = raw ("punch_makeup");

    s.pitchOn = on ("pitch_on");   s.semitones = raw ("pitch");
    s.pitchMix = raw ("pitch_mix"); s.fineCents = raw ("pitch_fine");

    s.colorOn = on ("color_on");   s.colorMode = mode ("color_mode");
    s.dirt = raw ("dirt");         s.colorTone = raw ("color_tone"); s.colorMix = raw ("color_mix");

    s.modOn = on ("mod_on");       s.modMode = mode ("mod_mode");
    s.modDepth = raw ("mod_depth"); s.modRate = raw ("mod_rate");    s.modMix = raw ("mod_mix");

    s.delayOn = on ("delay_on");   s.delayMode = mode ("delay_mode");
    s.delayFeedback = raw ("delay_feedback"); s.delayTone = raw ("delay_tone"); s.delayMix = raw ("delay_mix");
    s.throwOn = on ("throw");

    s.reverbOn = on ("reverb_on"); s.reverbMode = mode ("reverb_mode");
    s.reverbSize = raw ("reverb_size"); s.preDelayMs = raw ("reverb_predelay"); s.reverbMix = raw ("reverb_mix");

    s.space = raw ("space");
    s.width = raw ("width");

    if (auto* head = getPlayHead())
        if (auto pos = head->getPosition())
            if (auto bpm = pos->getBpm())
                s.bpm = *bpm;

    return s;
}

void SwellAdLibProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0)
        return;

    if (getTotalNumInputChannels() == 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, n);

    if (raw ("bypass") < 0.5f)
    {
        if (dryBuffer.getNumSamples() < n)
            dryBuffer.setSize (2, n, false, false, true);
        dryBuffer.copyFrom (0, 0, buffer, 0, 0, n);
        dryBuffer.copyFrom (1, 0, buffer, 1, 0, n);

        auto* L = buffer.getWritePointer (0);
        auto* R = buffer.getWritePointer (1);
        engine.process (L, R, n, readSettings());

        mixSmooth.setTargetValue (raw ("out_mix"));
        gainSmooth.setTargetValue (juce::Decibels::decibelsToGain (raw ("out_gain")));
        const auto* dL = dryBuffer.getReadPointer (0);
        const auto* dR = dryBuffer.getReadPointer (1);
        for (int i = 0; i < n; ++i)
        {
            const float mix = mixSmooth.getNextValue();
            const float gain = gainSmooth.getNextValue();
            L[i] = (dL[i] * (1.0f - mix) + L[i] * mix) * gain;
            R[i] = (dR[i] * (1.0f - mix) + R[i] * mix) * gain;
        }
    }

    // Meters and the LIVE display
    const float pl = buffer.getMagnitude (0, 0, n), pr = buffer.getMagnitude (1, 0, n);
    peakL.store (juce::jmax (pl, peakL.load()));
    peakR.store (juce::jmax (pr, peakR.load()));

    const auto* L = buffer.getReadPointer (0);
    const auto* R = buffer.getReadPointer (1);
    const int chunk = (int) (getSampleRate() * 0.02);
    for (int i = 0; i < n; ++i)
    {
        waveAcc = juce::jmax (waveAcc, std::abs (L[i] + R[i]) * 0.5f);
        if (++waveCount >= chunk)
        {
            const int w = waveWrite.load();
            wave[(size_t) w].store (waveAcc);
            waveWrite.store ((w + 1) % waveSize);
            waveAcc = 0.0f;
            waveCount = 0;
        }
    }
}

void SwellAdLibProcessor::loadPreset (int index)
{
    const auto& list = Presets::all();
    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (rp->getParameterID() != "bypass" && rp->getParameterID() != "out_gain")
                rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& v : list[(size_t) index].values)
        if (auto* rp = apvts.getParameter (v.id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (v.value));

    currentPreset.store (index);
}

void SwellAdLibProcessor::toggleAB()
{
    auto current = apvts.copyState();
    current.setProperty ("preset", currentPreset.load(), nullptr);
    (onB ? slotB : slotA) = current;
    onB = ! onB;

    const auto& other = onB ? slotB : slotA;
    if (other.isValid())
    {
        apvts.replaceState (other.createCopy());
        currentPreset.store ((int) other.getProperty ("preset", currentPreset.load()));
    }
}

void SwellAdLibProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset.load(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SwellAdLibProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentPreset.store ((int) state.getProperty ("preset", 0));
            apvts.replaceState (state);
        }
}

juce::AudioProcessorEditor* SwellAdLibProcessor::createEditor()
{
    return new SwellAdLibEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SwellAdLibProcessor();
}
