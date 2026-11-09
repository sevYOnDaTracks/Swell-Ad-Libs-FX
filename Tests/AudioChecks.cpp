#include "PluginProcessor.h"
#include "Presets.h"
#include <iostream>
#include <stdexcept>

namespace
{
using Audio = juce::AudioBuffer<float>;

void check (bool ok, const char* failure)
{
    if (!ok)
        throw std::runtime_error (failure);
}

void choose (SwellAdLibProcessor& p, const char* id, float value)
{
    auto* parameter = p.apvts.getParameter (id);
    check (parameter != nullptr, "missing parameter");
    parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

AdLib::Settings neutral()
{
    AdLib::Settings s;
    s.cleanOn = s.punchOn = false;
    s.space = 1;
    return s;
}

Audio phrase (int sr, double duration = 1.15)
{
    Audio b (2, (int)(sr * duration));
    b.clear();
    for (int i = sr / 10; i < sr * 35 / 100; ++i)
    {
        const double t = (double)i / sr;
        const float v =
            (float)(0.21 * std::sin (2 * 3.141592653589793 * 217 * t) + 0.09 * std::sin (2 * 3.141592653589793 * 653 * t));
        for (int ch = 0; ch < 2; ++ch)
            b.setSample (ch, i, v);
    }
    return b;
}

Audio render (const Audio& input, int sr, const AdLib::Settings& s, int block)
{
    AdLib::Engine engine;
    engine.prepare (sr);
    Audio out;
    out.makeCopyOf (input);
    for (int pos = 0; pos < out.getNumSamples(); pos += block)
        engine.process (out.getWritePointer (0, pos), out.getWritePointer (1, pos),
                        std::min (block, out.getNumSamples() - pos), s);
    return out;
}

double difference (const Audio& a, const Audio& b)
{
    double sum = 0;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < a.getNumSamples(); ++i)
        {
            const double d = a.getSample (ch, i) - b.getSample (ch, i);
            check (std::isfinite (d), "invalid audio sample");
            sum += d * d;
        }
    return std::sqrt (sum / (2 * a.getNumSamples()));
}

void modulation()
{
    for (int sr : {22050, 44100, 48000, 96000})
    {
        auto dry = phrase (sr);
        auto s = neutral();
        s.flangeOn = true;
        s.flangeMix = 0.82f;
        s.flangeDepth = 0.81f;
        auto sweep = render (dry, sr, s, 64);
        check (difference (sweep, dry) > 0.01, "flanger is silent or bypassed");
        check (difference (sweep, render (dry, sr, s, 2048)) < 0.00001, "host block size changed flanger");
        s.flangeMode = 1;
        auto stacked = render (dry, sr, s, 257);
        check (difference (sweep, stacked) > 0.01, "stacked voices sound identical to single tap");
        check (difference (stacked, render (dry, sr, s, 19)) < 0.00001, "unstable stacked phases");
        s.modOn = true;
        s.modMode = 1;
        s.modMix = 0.55f;
        check (difference (stacked, render (dry, sr, s, 512)) > 0.005, "chorus cannot be layered");
        s.modOn = false;
        s.flangeMix = 0;
        check (difference (dry, render (dry, sr, s, 512)) < 1e-7, "zero flange mix changed dry voice");
    }
}

void throwDelay()
{
    constexpr int sr = 48000;
    const auto dry = phrase (sr, 1.8);
    auto s = neutral();
    s.delayOn = true;
    s.throwOn = true;
    s.delayMix = 1;
    s.delayFeedback = 0;
    auto thrown = render (dry, sr, s, 128);
    // Last ~100 ms of a hard-cut syllable repeat at +1/4 note. Earlier vocal must stay out.
    check (thrown.getMagnitude (0, (int)(0.60 * sr), (int)(0.12 * sr)) < 1e-5, "Throw echoed the whole word");
    check (thrown.getRMSLevel (0, (int)(0.79 * sr), (int)(0.055 * sr)) > 0.05, "Throw lost a hard-cut syllable");
    check (thrown.getMagnitude (0, (int)(0.851 * sr), (int)(0.1 * sr)) < 1e-5, "Throw added timing latency");
    check (difference (thrown, render (dry, sr, s, 2048)) < 1e-6, "Throw changed across host block sizes");
    s.bpm = 300;
    s.delayMode = 2;
    auto shortThrow = render (dry, sr, s, 64);
    check (shortThrow.getMagnitude (0, (int)(0.35 * sr), (int)(0.05 * sr)) > 0.02, "shortest delay missed throw");
    s.throwOn = false;
    s.bpm = 120;
    s.delayMode = 4;
    s.delayFeedback = 0.55f;
    auto ping = render (dry, sr, s, 512);
    check (ping.getMagnitude (0, (int)(0.60 * sr), (int)(0.20 * sr)) > 0.1, "ping-pong left repeat");
    check (ping.getMagnitude (1, (int)(0.60 * sr), (int)(0.20 * sr)) < 1e-6, "ping-pong leaked right");
    check (ping.getMagnitude (1, (int)(1.10 * sr), (int)(0.2 * sr)) > 0.01, "missing second echo");
}

void ducking()
{
    const int sr = 48000;
    auto dry = phrase (sr, 1.5);
    dry.copyFrom (0, sr * 6 / 10, dry, 0, sr / 10, sr / 4);
    dry.copyFrom (1, sr * 6 / 10, dry, 1, sr / 10, sr / 4);
    for (bool testReverb : {false, true})
    {
        auto s = neutral();
        s.delayOn = !testReverb;
        s.reverbOn = testReverb;
        s.delayMix = s.reverbMix = 1;
        auto a = render (dry, sr, s, 257);
        s.delayDuck = s.reverbDuck = 1;
        auto b = render (dry, sr, s, 257);
        double normal = 0, ducked = 0;
        for (int i = sr * 65 / 100; i < sr * 8 / 10; ++i)
        {
            normal += std::pow (a.getSample (0, i) - dry.getSample (0, i), 2);
            ducked += std::pow (b.getSample (0, i) - dry.getSample (0, i), 2);
        }
        check (normal > 0.01 && ducked < normal * 0.50, "ducking did not clear space around the vocal");
    }
}

void compatibility()
{
    SwellAdLibProcessor p;
    check (p.apvts.getParameter ("mod_mode")->getAllValueStrings().size() == 3, "legacy automation range changed");
    choose (p, "mod_mode", 2);
    choose (p, "flange_on", 1);
    choose (p, "flange_mode", 1);
    choose (p, "delay_duck", 0.66f);
    juce::MemoryBlock current;
    p.getStateInformation (current);
    auto legacy = p.apvts.copyState();
    for (int i = legacy.getNumChildren(); --i >= 0;)
    {
        auto id = legacy.getChild (i)["id"].toString();
        if (id.startsWith ("flange_") || id.endsWith ("_duck"))
            legacy.removeChild (i, nullptr);
    }
    juce::MemoryBlock old;
    juce::AudioProcessor::copyXmlToBinary (*legacy.createXml(), old);
    p.setStateInformation (old.getData(), (int)old.getSize());
    check (p.apvts.getRawParameterValue ("flange_on")->load() == 0 &&
               p.apvts.getRawParameterValue ("delay_duck")->load() == 0,
           "legacy session inherited new processing");
    check (p.apvts.getRawParameterValue ("mod_mode")->load() == 2, "legacy Wide changed");
    p.setStateInformation (current.getData(), (int)current.getSize());
    check (p.apvts.getRawParameterValue ("flange_on")->load() == 1 &&
               p.apvts.getRawParameterValue ("flange_mode")->load() == 1,
           "new state not restored");
    int index = 0;
    for (const auto& preset : Presets::all())
    {
        p.loadPreset (index++);
        for (const auto& v : preset.values)
            if (auto actual = p.apvts.getRawParameterValue (v.id)->load();
                std::abs (actual - v.value) > std::max (0.0001f, std::abs (v.value) * 1e-5f))
                throw std::runtime_error (std::string (preset.name) + " / " + v.id + ": expected " +
                                          std::to_string (v.value) + ", got " + std::to_string (actual));
    }
}
} // namespace

int main()
{
    swell::Licence::unlockForTesting(); // rendu hors ligne, sans fichier de licence
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        modulation();
        throwDelay();
        ducking();
        compatibility();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::cout
        << "Passed: both flangers, stereo layers, block invariance, throw timing, ducking, legacy state and presets.\n";
}
