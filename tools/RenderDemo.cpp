// Renders before/after demos: runs a dry vocal WAV through every factory preset.
// Usage: SwellAdLibRender <dry.wav> <output folder> [bpm]
// Each output file plays the dry voice, a short gap, then the same take through the preset.

#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/AdLibEngine.h"
#include "../Source/Presets.h"

namespace
{
// Mirrors the plugin parameters: AdLib::Settings defaults are the parameter defaults
void apply (AdLib::Settings& s, const juce::String& id, float v)
{
    const bool on = v > 0.5f;
    const int mode = juce::roundToInt (v);
    if      (id == "space") s.space = v;
    else if (id == "dirt") s.dirt = v;
    else if (id == "pitch") s.semitones = v;
    else if (id == "width") s.width = v;
    else if (id == "clean_on") s.cleanOn = on;
    else if (id == "clean_mode") s.cleanMode = mode;
    else if (id == "clean_low") s.lowCut = v;
    else if (id == "clean_high") s.highCut = v;
    else if (id == "clean_deess") s.deess = v;
    else if (id == "punch_on") s.punchOn = on;
    else if (id == "punch_mode") s.punchMode = mode;
    else if (id == "punch_amount") s.punchAmount = v;
    else if (id == "punch_attack") s.punchAttackMs = v;
    else if (id == "punch_makeup") s.punchMakeupDb = v;
    else if (id == "pitch_on") s.pitchOn = on;
    else if (id == "pitch_mix") s.pitchMix = v;
    else if (id == "pitch_fine") s.fineCents = v;
    else if (id == "color_on") s.colorOn = on;
    else if (id == "color_mode") s.colorMode = mode;
    else if (id == "color_tone") s.colorTone = v;
    else if (id == "color_mix") s.colorMix = v;
    else if (id == "mod_on") s.modOn = on;
    else if (id == "mod_mode") s.modMode = mode;
    else if (id == "mod_depth") s.modDepth = v;
    else if (id == "mod_rate") s.modRate = v;
    else if (id == "mod_mix") s.modMix = v;
    else if (id == "delay_duck")     s.delayDuck = v;
    else if (id == "reverb_duck")    s.reverbDuck = v;
    else if (id == "flange_on")      s.flangeOn = on;
    else if (id == "flange_mode")    s.flangeMode = mode;
    else if (id == "flange_mix")     s.flangeMix = v;
    else if (id == "flange_depth")   s.flangeDepth = v;
    else if (id == "flange_rate")    s.flangeRate = v;
    else if (id == "flange_feedback") s.flangeFeedback = v;
    else if (id == "delay_on") s.delayOn = on;
    else if (id == "delay_mode") s.delayMode = mode;
    else if (id == "delay_feedback") s.delayFeedback = v;
    else if (id == "delay_tone") s.delayTone = v;
    else if (id == "delay_mix") s.delayMix = v;
    else if (id == "throw") s.throwOn = on;
    else if (id == "reverb_on") s.reverbOn = on;
    else if (id == "reverb_mode") s.reverbMode = mode;
    else if (id == "reverb_size") s.reverbSize = v;
    else if (id == "reverb_predelay") s.preDelayMs = v;
    else if (id == "reverb_mix") s.reverbMix = v;
    else std::cerr << "Unknown parameter: " << id << "\n";
}

void normalise (juce::AudioBuffer<float>& b, float targetDb)
{
    const float peak = b.getMagnitude (0, b.getNumSamples());
    if (peak > 0.0f)
        b.applyGain (juce::Decibels::decibelsToGain (targetDb) / peak);
}

bool writeWav (const juce::File& file, const juce::AudioBuffer<float>& b, double sr)
{
    file.deleteFile();
    std::unique_ptr<juce::OutputStream> out (file.createOutputStream().release());
    if (out == nullptr)
        return false;
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (out.get(), sr, (unsigned int) b.getNumChannels(), 16, {}, 0));
    if (writer == nullptr)
        return false;
    out.release();
    return writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}
} // namespace

int main (int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: SwellAdLibRender <dry.wav> <output folder> [bpm]\n";
        return 1;
    }

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (juce::File (argv[1])));
    if (reader == nullptr)
    {
        std::cerr << "Cannot read " << argv[1] << "\n";
        return 1;
    }

    const double sr = reader->sampleRate;
    const int len = (int) reader->lengthInSamples;
    const int tail = (int) (sr * 3.0), gap = (int) (sr * 0.8);

    juce::AudioBuffer<float> dry (2, len);
    reader->read (&dry, 0, len, 0, true, true);
    if (reader->numChannels == 1)
        dry.copyFrom (1, 0, dry, 0, 0, len);
    normalise (dry, -3.0f);

    const juce::File outDir (argv[2]);
    outDir.createDirectory();
    const double bpm = argc > 3 ? juce::String (argv[3]).getDoubleValue() : 140.0;

    const auto& list = Presets::all();
    for (size_t p = 0; p < list.size(); ++p)
    {
        AdLib::Settings s;
        s.bpm = bpm;
        for (const auto& v : list[p].values)
            apply (s, v.id, v.value);

        juce::AudioBuffer<float> wet (2, len + tail);
        wet.clear();
        wet.copyFrom (0, 0, dry, 0, 0, len);
        wet.copyFrom (1, 0, dry, 1, 0, len);

        AdLib::Engine engine;
        engine.prepare (sr);
        engine.process (wet.getWritePointer (0), wet.getWritePointer (1), wet.getNumSamples(), s);
        normalise (wet, -3.0f);

        juce::AudioBuffer<float> both (2, len + gap + wet.getNumSamples());
        both.clear();
        for (int c = 0; c < 2; ++c)
        {
            both.copyFrom (c, 0, dry, c, 0, len);
            both.copyFrom (c, len + gap, wet, c, 0, wet.getNumSamples());
        }

        const auto name = juce::String (p + 1).paddedLeft ('0', 2) + " - " + list[p].name + " (avant-apres).wav";
        const bool ok = writeWav (outDir.getChildFile (name), both, sr);
        std::cout << (ok ? "OK   " : "FAIL ") << name << "\n";
    }
    return 0;
}
