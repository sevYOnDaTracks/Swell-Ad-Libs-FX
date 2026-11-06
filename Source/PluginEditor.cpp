#include "PluginEditor.h"
#include "Presets.h"

using namespace juce;
using namespace SwellColours;

namespace
{
constexpr int mainWidth = 800, mainHeight = 390;
const Colour accents[]{Colour (0xff91b9e1), Colour (0xffbda5e2), Colour (0xffe9ae7d), Colour (0xff8cc7bd)};
constexpr int quickStart = 5;
const EffectSpec groups[]{
    {"Macros voix",
     nullptr,
     nullptr,
     {{{"Espace", "space"}, {"Saturation", "dirt"}, {"Pitch", "pitch"}, {"Largeur", "width"}}}},
    {"Filtre",
     "clean_on",
     "clean_mode",
     {{{"Coupe-bas", "clean_low"}, {"Coupe-haut", "clean_high"}, {"De-esser", "clean_deess"}, {}}}},
    {"Compression",
     "punch_on",
     "punch_mode",
     {{{"Intensité", "punch_amount"}, {"Attaque", "punch_attack"}, {"Gain", "punch_makeup"}, {}}}},
    {"Pitch", "pitch_on", nullptr, {{{"Demi-tons", "pitch"}, {"Accord fin", "pitch_fine"}, {"Mix", "pitch_mix"}, {}}}},
    {"Couleur", "color_on", "color_mode", {{{"Drive", "dirt"}, {"Tonalité", "color_tone"}, {"Mix", "color_mix"}, {}}}},
    {"Modulation", "mod_on", "mod_mode", {{{"Profondeur", "mod_depth"}, {"Vitesse", "mod_rate"}, {"Mix", "mod_mix"}, {}}}},
    {"Flange",
     "flange_on",
     "flange_mode",
     {{{"Profondeur", "flange_depth"},
       {"Vitesse", "flange_rate"},
       {"Feedback", "flange_feedback"},
       {"Mix", "flange_mix"}}}},
    {"Delay",
     "delay_on",
     "delay_mode",
     {{{"Feedback", "delay_feedback"}, {"Tonalité", "delay_tone"}, {"Mix", "delay_mix"}, {"Ducking", "delay_duck"}}}},
    {"Reverb",
     "reverb_on",
     "reverb_mode",
     {{{"Taille", "reverb_size"}, {"Pré-delay", "reverb_predelay"}, {"Mix", "reverb_mix"}, {"Ducking", "reverb_duck"}}}}};
const char* const quickMix[]{"mod_mix", "flange_mix", "delay_mix", "reverb_mix"};

float value (SwellAdLibProcessor& proc, const char* id)
{
    return proc.apvts.getRawParameterValue (id)->load();
}

void setValue (SwellAdLibProcessor& proc, const char* id, float real)
{
    if (auto* param = proc.apvts.getParameter (id))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (real));
        param->endChangeGesture();
    }
}

void enableEffect (SwellAdLibProcessor& proc, int index)
{
    if (const char* id = groups[index].power; id && value (proc, id) < 0.5f)
        setValue (proc, id, 1);
    // Delay and reverb also share the original SPACE master.
    if ((index == 7 || index == 8) && value (proc, "space") == 0.0f)
        setValue (proc, "space", 1);
}

void setModes (ComboBox& box, SwellAdLibProcessor& proc, const char* id)
{
    box.clear (dontSendNotification);
    if (id)
        box.addItemList (proc.apvts.getParameter (id)->getAllValueStrings(), 1);
}

void refreshPower (TextButton& power, bool on)
{
    const String name = on ? "On" : "Off";
    if (power.getButtonText() != name)
        power.setButtonText (name);
    power.setToggleState (on, dontSendNotification);
}

void caption (Graphics& g, const String& s, Rectangle<int> r)
{
    g.setColour (dim);
    g.setFont (swellFont (11.5f, true));
    g.drawText (s, r, Justification::centredLeft);
}
} // namespace

ParameterKnob::ParameterKnob (SwellAdLibProcessor& p) : proc (p)
{
    dial.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    dial.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    dial.setRotaryParameters (MathConstants<float>::pi * 1.24f, MathConstants<float>::pi * 2.76f, true);
    dial.setMouseDragSensitivity (185);
    dial.onValueChange = [this] { repaint(); };
    addAndMakeVisible (dial);
}

void ParameterKnob::bind (const ControlSpec& spec, Colour colour, std::function<void()> enable)
{
    attachment.reset();
    label = String::fromUTF8 (spec.name);
    ink = colour;
    dial.setName (label.isNotEmpty() ? label : proc.apvts.getParameter (spec.parameter)->getName (60));
    dial.setTooltip (dial.getName());
    dial.setColour (Slider::rotarySliderFillColourId, ink);
    dial.onDragStart = std::move (enable);
    attachment = std::make_unique<SliderAttachment> (proc.apvts, spec.parameter, dial);
    repaint();
}

void ParameterKnob::resized()
{
    auto r = getLocalBounds().withTrimmedTop (label.isEmpty() ? 5 : 16).withTrimmedBottom (20);
    int size = jmin (r.getWidth(), r.getHeight());
    dial.setBounds (r.withSizeKeepingCentre (size, size));
}

void ParameterKnob::paint (Graphics& g)
{
    g.setFont (swellFont (12.0f));
    g.setColour (dim);
    if (label.isNotEmpty())
        g.drawText (label, getLocalBounds().removeFromTop (16), Justification::centred);
    g.setColour (ink);
    g.setFont (swellFont (15.0f, true));
    g.drawText (dial.getTextFromValue (dial.getValue()), getLocalBounds().removeFromBottom (20), Justification::centred);
}

EffectStrip::EffectStrip (SwellAdLibProcessor& p, int effect) : proc (p), index (effect), amount (p)
{
    const auto& spec = groups[index];
    power.setName (String (spec.name) + " On/Off");
    power.onClick = [this]
    {
        if (value (proc, groups[index].power) > 0.5f)
            setValue (proc, groups[index].power, 0);
        else
            enableEffect (proc, index);
        refresh();
    };
    setModes (mode, proc, spec.mode);
    mode.setName (String (spec.name) + " Mode");
    mode.onChange = [this]
    {
        setValue (proc, groups[index].mode, (float)mode.getSelectedItemIndex());
        enableEffect (proc, index);
        refresh();
    };
    amount.bind ({"", quickMix[index - quickStart]}, accents[index - quickStart], [this] { enableEffect (proc, index); });
    addAndMakeVisible (power);
    addAndMakeVisible (mode);
    addAndMakeVisible (amount);
    refresh();
}

void EffectStrip::refresh()
{
    bool on = value (proc, groups[index].power) > 0.5f;
    if (on != active)
    {
        active = on;
        repaint();
    }
    amount.setAlpha (on ? 1.0f : 0.65f);
    refreshPower (power, on);
    mode.setSelectedId (1 + roundToInt (value (proc, groups[index].mode)), dontSendNotification);
}

void EffectStrip::resized()
{
    power.setBounds (getWidth() - 49, 10, 40, 23);
    mode.setBounds (10, 43, getWidth() - 20, 27);
    amount.setBounds (10, 82, getWidth() - 20, getHeight() - 91);
}

void EffectStrip::paint (Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (panel);
    g.fillRoundedRectangle (r, 8);
    g.setColour (active ? accents[index - quickStart].withAlpha (0.58f) : border);
    g.drawRoundedRectangle (r, 8, 1);
    g.setColour (active ? accents[index - quickStart] : dim);
    g.setFont (swellFont (15.0f, true));
    g.drawText (groups[index].name, 11, 10, getWidth() - 65, 23, Justification::centredLeft);
}

DetailView::DetailView (SwellAdLibProcessor& p) : proc (p)
{
    for (int i = 0; i < (int)std::size (groups); ++i)
        group.addItem (String::fromUTF8 (groups[i].name), i + 1);
    group.setName ("Section");
    group.onChange = [this] { select (group.getSelectedItemIndex()); };
    mode.onChange = [this]
    {
        if (groups[selected].mode)
            setValue (proc, groups[selected].mode, (float)mode.getSelectedItemIndex());
        else if (selected == 3 && mode.getSelectedId() <= 4)
        {
            constexpr int steps[]{-12, -7, 7, 12};
            setValue (proc, "pitch", (float)steps[mode.getSelectedItemIndex()]);
        }
        enableEffect (proc, selected);
    };
    power.onClick = [this]
    {
        const char* id = groups[selected].power;
        if (!id)
            return;
        if (value (proc, id) > 0.5f)
            setValue (proc, id, 0);
        else
            enableEffect (proc, selected);
        refresh();
    };
    addAndMakeVisible (group);
    addAndMakeVisible (mode);
    addAndMakeVisible (power);
    for (int i = 0; i < 4; ++i)
        addAndMakeVisible (knobs.add (new ParameterKnob (proc)));
    select (0);
}

void DetailView::select (int index)
{
    if (!isPositiveAndBelow (index, (int)std::size (groups)))
        return;
    selected = index;
    group.setSelectedId (index + 1, dontSendNotification);
    const auto& spec = groups[index];
    setModes (mode, proc, spec.mode);
    if (index == 3)
        mode.addItemList ({"-12 st", "-7 st", "+7 st", "+12 st", "Libre"}, 1);
    mode.setVisible (spec.mode != nullptr || index == 3);
    power.setVisible (spec.power != nullptr);
    for (int i = 0; i < knobs.size(); ++i)
    {
        const auto& c = spec.controls[(size_t)i];
        knobs[i]->setVisible (c.parameter != nullptr);
        if (c.parameter)
            knobs[i]->bind (c, accentBlue,
                            [this, i]
                            {
                                if (selected != 0)
                                    enableEffect (proc, selected);
                                else if (i == 0 && value (proc, "delay_on") < 0.5f && value (proc, "reverb_on") < 0.5f)
                                    setValue (proc, "reverb_on", 1);
                                else if (i == 1)
                                    enableEffect (proc, 4);
                                else if (i == 2)
                                    enableEffect (proc, 3);
                            });
    }
    refresh();
    resized();
    repaint();
}

void DetailView::refresh()
{
    const auto& spec = groups[selected];
    if (spec.mode)
        mode.setSelectedId (1 + roundToInt (value (proc, spec.mode)), dontSendNotification);
    else if (selected == 3)
    {
        int pitch = roundToInt (value (proc, "pitch"));
        constexpr int shortcut[] = {-12, -7, 7, 12};
        int id = 5;
        for (int k = 0; k < 4; ++k)
            if (shortcut[k] == pitch)
                id = k + 1;
        mode.setSelectedId (id, dontSendNotification);
    }
    if (spec.power)
        refreshPower (power, value (proc, spec.power) > 0.5f);
}

void DetailView::resized()
{
    group.setBounds (15, 13, 185, 29);
    mode.setBounds (214, 13, 159, 29);
    power.setBounds (getWidth() - 70, 13, 55, 29);
    int count = 0;
    for (auto* k : knobs)
        if (k->isVisible())
            ++count;
    if (count == 0)
        return;
    const int w = (getWidth() - 24) / count;
    for (int i = 0; i < count; ++i)
        knobs[i]->setBounds (12 + i * w, 57, w, getHeight() - 67);
}

void DetailView::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (panel);
    g.fillRoundedRectangle (r, 8);
    g.setColour (border);
    g.drawRoundedRectangle (r, 8, 1);
}

SwellAdLibEditor::SwellAdLibEditor (SwellAdLibProcessor& p) : AudioProcessorEditor (&p), proc (p), detail (p)
{
    setLookAndFeel (&lnf);
    const auto& bank = Presets::all();
    for (auto* category : Presets::categories)
    {
        PopupMenu list;
        for (int i = 0; i < (int)bank.size(); ++i)
            if (String (bank[(size_t)i].category) == category)
                list.addItem (i + 1, bank[(size_t)i].name);
        presets.getRootMenu()->addSubMenu (category, list);
    }
    presets.setName ("Preset");
    presets.onChange = [this]
    {
        if (presets.getSelectedId() > 0)
            proc.loadPreset (presets.getSelectedId() - 1);
        refresh();
    };
    auto step = [this] (int delta)
    {
        int n = (int)Presets::all().size();
        proc.loadPreset ((proc.getCurrentPreset() + delta + n) % n);
        refresh();
    };
    previous.onClick = [step] { step (-1); };
    next.onClick = [step] { step (1); };
    previous.setTooltip ("Preset précédent");
    next.setTooltip ("Preset suivant");
    compare.onClick = [this]
    {
        proc.toggleAB();
        refresh();
    };
    compare.setTooltip ("Comparer deux réglages");
    bypass.onClick = [this]
    {
        setValue (proc, "bypass", value (proc, "bypass") < 0.5f ? 1.0f : 0.0f);
        refresh();
    };
    zoom.setTooltip ("Taille de la fenêtre");
    zoom.onClick = [this] { zoomMenu(); };
    detailsButton.onClick = [this] { showDetails (!detailsShown); };
    throwButton.setTooltip ("Répéter seulement la dernière syllabe, au tempo du projet");
    throwButton.onClick = [this]
    {
        const bool turnOn = value (proc, "throw") < 0.5f;
        if (turnOn)
            enableEffect (proc, 7);
        setValue (proc, "throw", turnOn ? 1.0f : 0.0f);
        refresh();
    };
    for (auto* b : {&previous, &next, &compare, &bypass, &detailsButton, &throwButton, &zoom})
        addAndMakeVisible (*b);
    addAndMakeVisible (presets);
    for (int i = 0; i < 4; ++i)
        addAndMakeVisible (strips.add (new EffectStrip (proc, quickStart + i)));
    addChildComponent (detail);
    for (auto* s : {&mix, &gain})
    {
        s->setSliderStyle (Slider::LinearHorizontal);
        s->setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        s->onValueChange = [this] { repaint (175, 320, 425, 65); };
        addAndMakeVisible (*s);
    }
    mix.setName ("Mix global");
    gain.setName ("Gain de sortie");
    mixAttachment = std::make_unique<SliderAttachment> (proc.apvts, "out_mix", mix);
    gainAttachment = std::make_unique<SliderAttachment> (proc.apvts, "out_gain", gain);
    showDetails (false);
    setSize (mainWidth, mainHeight);
    refresh();
    startTimerHz (24);
}

SwellAdLibEditor::~SwellAdLibEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SwellAdLibEditor::showDetails (bool show)
{
    detailsShown = show;
    for (auto* strip : strips)
        strip->setVisible (!show);
    detail.setVisible (show);
    detailsButton.setButtonText (show ? String::fromUTF8 ("← Effets") : String::fromUTF8 ("Réglages"));
    detailsButton.setToggleState (show, dontSendNotification);
    repaint();
}

void SwellAdLibEditor::zoomMenu()
{
    auto* frame = dynamic_cast<AudioProcessorEditor*> (getParentComponent());
    if (!frame)
        return;
    PopupMenu m;
    for (int percent : {75, 100, 125, 150})
        m.addItem (percent, String (percent) + " %", true, frame->getWidth() == mainWidth * percent / 100);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&zoom),
                     [safe = Component::SafePointer<AudioProcessorEditor> (frame)] (int result)
                     {
                         if (safe != nullptr && result > 0)
                             safe->setBoundsConstrained (safe->getBounds().withSizeKeepingCentre (
                                 mainWidth * result / 100, mainHeight * result / 100));
                     });
}

void SwellAdLibEditor::refresh()
{
    const int index = proc.getCurrentPreset();
    if (shownPreset != index)
    {
        shownPreset = index;
        presets.setSelectedId (index + 1, dontSendNotification);
        repaint();
    }
    for (auto* strip : strips)
        strip->refresh();
    if (detailsShown)
        detail.refresh();
    compare.setButtonText (proc.isOnB() ? "B" : "A");
    bypass.setToggleState (value (proc, "bypass") > 0.5f, dontSendNotification);
    throwButton.setToggleState (value (proc, "throw") > 0.5f, dontSendNotification);
    if (auto* frame = getParentComponent())
        zoom.setButtonText (String (roundToInt (100.0 * frame->getWidth() / mainWidth)) + "%");
}

void SwellAdLibEditor::timerCallback()
{
    refresh();
    levels[0] = jmax (levels[0] * 0.84f, proc.peakL.exchange (0.0f));
    levels[1] = jmax (levels[1] * 0.84f, proc.peakR.exchange (0.0f));
    repaint (627, 339, 145, 29);
}

void SwellAdLibEditor::resized()
{
    presets.setBounds (272, 16, 235, 31);
    previous.setBounds (514, 16, 30, 31);
    next.setBounds (546, 16, 30, 31);
    compare.setBounds (596, 16, 33, 31);
    bypass.setBounds (634, 16, 70, 31);
    zoom.setBounds (714, 16, 62, 31);
    detailsButton.setBounds (677, 76, 99, 27);
    const int gap = 8, w = (mainWidth - 48 - gap * 3) / 4;
    for (int i = 0; i < strips.size(); ++i)
        strips[i]->setBounds (24 + i * (w + gap), 112, w, 191);
    detail.setBounds (24, 112, mainWidth - 48, 191);
    throwButton.setBounds (24, 333, 115, 33);
    mix.setBounds (174, 347, 159, 20);
    gain.setBounds (377, 347, 160, 20);
}

void SwellAdLibEditor::paint (Graphics& g)
{
    g.fillAll (background);
    g.setGradientFill (ColourGradient (panelLight, 0, 0, background, (float)getWidth(), 0, false));
    g.fillRect (0, 0, getWidth(), 61);
    g.setColour (border);
    g.drawHorizontalLine (61, 0, (float)getWidth());
    drawSwellLogo (g, {24.0f, 15.0f, 31.0f, 31.0f});
    g.setColour (text);
    g.setFont (swellFont (21, true));
    g.drawText ("Ad-lib FX", 65, 17, 154, 26, Justification::centredLeft);
    g.setColour (dim);
    g.setFont (swellFont (10.0f, true));
    g.drawText ("PRESET", 221, 16, 46, 31, Justification::centredLeft);

    g.setFont (swellFont (18, true));
    g.setColour (text);
    g.drawText (detailsShown ? String::fromUTF8 ("Réglages") : String ("Effets"), 24, 73, 112, 30,
                Justification::centredLeft);
    if (isPositiveAndBelow (shownPreset, (int)Presets::all().size()))
    {
        g.setFont (swellFont (12.0f));
        g.setColour (dim);
        g.drawFittedText (String::fromUTF8 (Presets::all()[(size_t)shownPreset].description), 135, 75, 525, 27,
                          Justification::centredLeft, 2);
    }
    g.setColour (border);
    g.drawHorizontalLine (317, 24, 776);
    caption (g, "MIX", {174, 327, 50, 18});
    caption (g, "GAIN", {377, 327, 50, 18});
    g.setColour (text);
    g.setFont (swellFont (12.0f, true));
    g.drawText (mix.getTextFromValue (mix.getValue()), 225, 327, 108, 18, Justification::centredRight);
    g.drawText (gain.getTextFromValue (gain.getValue()), 433, 327, 104, 18, Justification::centredRight);
    caption (g, "SORTIE", {627, 322, 75, 18});
    for (int ch = 0; ch < 2; ++ch)
    {
        Rectangle<float> bar (627.0f, 343.0f + 9.0f * ch, 149.0f, 5.0f);
        g.setColour (track);
        g.fillRoundedRectangle (bar, 2);
        const float db = Decibels::gainToDecibels (levels[ch], -54.0f);
        float fraction = jlimit (0.0f, 1.0f, (db + 54.0f) / 54.0f);
        g.setColour (db > 0 ? orange : accentBlue);
        g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * fraction), 2);
    }
    g.setColour (dim);
    g.setFont (swellFont (10.5f));
    g.drawText ("SWELL AUDIO  /  " JucePlugin_VersionString, 24, 372, 255, 13, Justification::centredLeft);
    g.setFont (swellFont (11.5f));
    g.drawText ("by sevyondatracks", getWidth() - 24 - 180, 371, 180, 15, Justification::centredRight);
}
