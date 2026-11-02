#include "PluginEditor.h"
#include "Presets.h"

namespace
{
juce::String utf8 (const char* s) { return juce::String::fromUTF8 (s); }

struct ModuleInfo
{
    const char* name;
    const char* hint;
    const char* onId;
    const char* modeId;  // nullptr: PITCH, whose modes set the semitones
    const char* params[3];
    const char* labels[3];
    const char* icon;    // SVG path in a 28 x 28 box
};

const ModuleInfo modules[] = {
    { "CLEAN", "filtres + de-esser", "clean_on", "clean_mode",
      { "clean_low", "clean_high", "clean_deess" }, { "Coupe basses", "Coupe aigus", "De-esser" },
      "M3 22c5 0 6-13 11-13h11 M3 25h22" },
    { "PUNCH", "compresseur", "punch_on", "punch_mode",
      { "punch_amount", "punch_attack", "punch_makeup" }, { "Quantit\xc3\xa9", "Attaque", "Volume" },
      "M14 2v8M10 6l4 4 4-4 M14 26v-8M10 22l4-4 4 4 M4 14h20" },
    { "PITCH", "octave et transposition", "pitch_on", nullptr,
      { "pitch", "pitch_mix", "pitch_fine" }, { "Transposition", "Mix", "Fin" },
      "M8 23V5M4 9l4-4 4 4 M20 5v18M16 19l4 4 4-4" },
    { "COLOR", "saturation et effets de caract\xc3\xa8" "re", "color_on", "color_mode",
      { "dirt", "color_tone", "color_mix" }, { "Drive (Dirt)", "Tone", "Mix" },
      "M2 14h3l2-7h5l2 14h5l2-7h5" },
    { "MOD", "\xc3\xa9" "paisseur et largeur", "mod_on", "mod_mode",
      { "mod_depth", "mod_rate", "mod_mix" }, { "Profondeur", "Vitesse", "Mix" },
      "M2 12c3-6 6-6 9 0s6 6 9 0 4-4 6-2 M2 18c3-6 6-6 9 0s6 6 9 0 4-4 6-2" },
    { "DELAY", "\xc3\xa9" "cho cal\xc3\xa9 sur le tempo", "delay_on", "delay_mode",
      { "delay_feedback", "delay_tone", "delay_mix" }, { "Feedback", "Tone", "Mix" },
      "M4 5v18 M11 8v12 M18 11v6 M24 13v2" },
    { "REVERB", "espace", "reverb_on", "reverb_mode",
      { "reverb_size", "reverb_predelay", "reverb_mix" }, { "Taille", "Pre-delay", "Mix" },
      "M3.5 23a1.5 1.5 0 1 0 3 0a1.5 1.5 0 1 0 -3 0 M5 16a7 7 0 0 1 7 7 M5 10a13 13 0 0 1 13 13 M5 4a19 19 0 0 1 19 19" },
};

const int pitchSteps[] = { -12, -7, 7, 12 };

bool paramOn (SwellAdLibProcessor& p, const char* id) { return p.apvts.getRawParameterValue (id)->load() > 0.5f; }

void setParam (SwellAdLibProcessor& p, const char* id, float realValue)
{
    if (auto* rp = p.apvts.getParameter (id))
    {
        rp->beginChangeGesture();
        rp->setValueNotifyingHost (rp->convertTo0to1 (realValue));
        rp->endChangeGesture();
    }
}

void drawSectionLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> r)
{
    g.setColour (SwellColours::dim);
    g.setFont (swellFont (11.0f, true));
    g.drawText (text, r, juce::Justification::centredLeft);
}
} // namespace

//==============================================================================
void LiveDisplay::paint (juce::Graphics& g)
{
    using namespace SwellColours;
    const auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (well);
    g.fillRoundedRectangle (r, 12.0f);
    g.setColour (border);
    g.drawRoundedRectangle (r, 12.0f, 1.0f);

    constexpr int bars = 96;
    const float barW = 4.0f, gap = 2.5f;
    const float total = bars * barW + (bars - 1) * gap;
    float x = r.getCentreX() - total * 0.5f;
    const float cy = r.getCentreY() + 6.0f;
    const int newest = proc.waveWrite.load();

    for (int k = 0; k < bars; ++k)
    {
        const int idx = (newest - bars + k + SwellAdLibProcessor::waveSize * 2) % SwellAdLibProcessor::waveSize;
        const float v = juce::jlimit (0.0f, 1.0f, std::sqrt (proc.wave[(size_t) idx].load()));
        const float h = 4.0f + v * (r.getHeight() - 30.0f);
        g.setColour (k % 2 == 0 ? orange : juce::Colour (0xffb8622c));
        g.fillRoundedRectangle (x, cy - h * 0.5f, barW, h, 2.0f);
        x += barW + gap;
    }

    g.setColour (orange);
    g.fillEllipse (14.0f, 12.0f, 6.0f, 6.0f);
    g.setColour (text);
    g.setFont (swellFont (10.0f, true));
    g.drawText ("LIVE", 26, 8, 60, 14, juce::Justification::centredLeft);
}

//==============================================================================
namespace
{
const char* const macroIds[]   = { "space", "dirt", "pitch", "width" };
const char* const macroNames[] = { "SPACE", "DIRT", "PITCH", "WIDTH" };
const char* const macroHints[] = { "reverb + delay", "saturation (Color)", "octave - / + (Pitch)", "largeur st\xc3\xa9r\xc3\xa9o" };
} // namespace

MacroPanel::MacroPanel (SwellAdLibProcessor& p) : proc (p)
{
    for (int i = 0; i < 4; ++i)
    {
        auto& k = knobs[i];
        k.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        k.setMouseDragSensitivity (220);
        k.onValueChange = [this] { repaint(); };
        k.onDragStart = [this, i] { ensureModuleOn (i); };
        addAndMakeVisible (k);
        attachments[i] = std::make_unique<SliderAttachment> (proc.apvts, macroIds[i], k);
    }
}

// Moving a macro whose effect is off switches that effect on, so the knob always does something
void MacroPanel::ensureModuleOn (int macro)
{
    if (macro == 0 && ! paramOn (proc, "delay_on") && ! paramOn (proc, "reverb_on")) setParam (proc, "reverb_on", 1.0f);
    if (macro == 1 && ! paramOn (proc, "color_on")) setParam (proc, "color_on", 1.0f);
    if (macro == 2 && ! paramOn (proc, "pitch_on")) setParam (proc, "pitch_on", 1.0f);
}

juce::Rectangle<int> MacroPanel::card (int i) const
{
    const int gap = 14;
    const int w = (getWidth() - gap * 3) / 4;
    return { i * (w + gap), 0, w, getHeight() };
}

void MacroPanel::resized()
{
    for (int i = 0; i < 4; ++i)
        knobs[i].setBounds (juce::Rectangle<int> (124, 124).withCentre ({ card (i).getCentreX(), 108 }));
}

void MacroPanel::paint (juce::Graphics& g)
{
    using namespace SwellColours;
    for (int i = 0; i < 4; ++i)
    {
        const auto c = card (i);
        g.setColour (panel);
        g.fillRoundedRectangle (c.toFloat().reduced (0.5f), 14.0f);
        g.setColour (border);
        g.drawRoundedRectangle (c.toFloat().reduced (0.5f), 14.0f, 1.0f);

        g.setColour (text);
        g.setFont (swellFont (14.0f, true));
        g.drawText (macroNames[i], c.withY (16).withHeight (20), juce::Justification::centred);

        g.setColour (orange);
        g.setFont (swellFont (19.0f, true));
        g.drawText (knobs[i].getTextFromValue (knobs[i].getValue()), c.withY (180).withHeight (24), juce::Justification::centred);

        g.setColour (dim);
        g.setFont (swellFont (11.5f));
        g.drawText (utf8 (macroHints[i]), c.withY (212).withHeight (18), juce::Justification::centred);
    }
}

//==============================================================================
ModuleButton::ModuleButton (const juce::String& n, const juce::String& svgIcon)
    : name (n), icon (juce::Drawable::parseSVGPath (svgIcon))
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void ModuleButton::setState (bool isOn, bool isSelected)
{
    if (isOn == on && isSelected == selected)
        return;
    on = isOn;
    selected = isSelected;
    repaint();
}

void ModuleButton::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && onClick)
        onClick();
}

void ModuleButton::paint (juce::Graphics& g)
{
    using namespace SwellColours;
    const auto r = getLocalBounds().toFloat().reduced (1.0f);

    if (on)
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2420), 0.0f, r.getY(), juce::Colour (0xff201d1b), 0.0f, r.getBottom(), false));
    else
        g.setColour (juce::Colour (0xff1c1c20));
    g.fillRoundedRectangle (r, 12.0f);

    if (isMouseOver())
    {
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.fillRoundedRectangle (r, 12.0f);
    }

    g.setColour (selected ? orange : on ? orangeDeep : border);
    g.drawRoundedRectangle (r, 12.0f, selected ? 2.0f : 1.0f);

    g.setColour (on ? orange : juce::Colour (0xff3a3a41));
    g.fillEllipse (r.getRight() - 15.0f, r.getY() + 8.0f, 7.0f, 7.0f);

    const float size = 30.0f;
    const auto iconArea = juce::Rectangle<float> (size, size).withCentre ({ r.getCentreX(), r.getCentreY() - 10.0f });
    auto p = icon;
    p.applyTransform (juce::AffineTransform::scale (size / 28.0f).translated (iconArea.getX(), iconArea.getY()));
    g.setColour (on ? orange : juce::Colour (0xff6c6c75));
    g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (on || selected ? text : dim);
    g.setFont (swellFont (12.0f, true));
    g.drawText (name, r.withTop (r.getCentreY() + 14.0f).withHeight (18.0f), juce::Justification::centred);
}

//==============================================================================
ModulePanel::ModulePanel (SwellAdLibProcessor& p) : proc (p)
{
    back.onClick = [this] { if (onBack) onBack(); };
    addAndMakeVisible (back);

    power.setClickingTogglesState (true);
    addAndMakeVisible (power);

    for (auto& s : sliders)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.onValueChange = [this] { repaint(); };
        addAndMakeVisible (s);
    }
}

void ModulePanel::setModule (int index)
{
    module = index;
    const auto& info = modules[index];

    powerAtt.reset();
    powerAtt = std::make_unique<ButtonAttachment> (proc.apvts, info.onId, power);

    for (int k = 0; k < 3; ++k)
    {
        sliderAtt[k].reset();
        sliderAtt[k] = std::make_unique<SliderAttachment> (proc.apvts, info.params[k], sliders[k]);
    }

    modeButtons.clear();
    juce::StringArray names;
    if (info.modeId != nullptr)
        names = proc.apvts.getParameter (info.modeId)->getAllValueStrings();
    else
        names = { "-12 st", "-7 st", "+7 st", "+12 st", "Libre" };

    for (int k = 0; k < names.size(); ++k)
    {
        auto* b = modeButtons.add (new juce::TextButton (names[k]));
        b->getProperties().set ("corner", 18.0f);
        b->onClick = [this, k] { pickMode (k); };
        addAndMakeVisible (b);
    }

    refresh();
    resized();
    repaint();
}

void ModulePanel::pickMode (int k)
{
    const auto& info = modules[module];
    if (info.modeId != nullptr)
        setParam (proc, info.modeId, (float) k);
    else if (k < 4)
        setParam (proc, "pitch", (float) pitchSteps[k]);

    if (! paramOn (proc, info.onId))
        setParam (proc, info.onId, 1.0f);
    refresh();
}

void ModulePanel::refresh()
{
    const auto& info = modules[module];
    int active = 0;
    if (info.modeId != nullptr)
    {
        active = juce::roundToInt (proc.apvts.getRawParameterValue (info.modeId)->load());
    }
    else
    {
        const int st = juce::roundToInt (proc.apvts.getRawParameterValue ("pitch")->load());
        active = 4;
        for (int k = 0; k < 4; ++k)
            if (pitchSteps[k] == st) active = k;
    }

    for (int k = 0; k < modeButtons.size(); ++k)
        modeButtons[k]->setToggleState (k == active, juce::dontSendNotification);

    power.setButtonText (power.getToggleState() ? "ON" : "OFF");
}

void ModulePanel::resized()
{
    back.setBounds (22, 18, 100, 36);
    power.setBounds (getWidth() - 22 - 80, 18, 80, 36);

    int x = 22;
    for (auto* b : modeButtons)
    {
        const int w = juce::roundToInt (juce::GlyphArrangement::getStringWidth (swellFont (13.0f, true), b->getButtonText())) + 30;
        b->setBounds (x, 96, w, 36);
        x += w + 6;
    }

    const int colW = (getWidth() - 44 - 48) / 3;
    for (int k = 0; k < 3; ++k)
        sliders[k].setBounds (22 + k * (colW + 24), 196, colW, 26);
}

void ModulePanel::paint (juce::Graphics& g)
{
    using namespace SwellColours;
    const auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (panel);
    g.fillRoundedRectangle (r, 14.0f);
    g.setColour (orangeDeep);
    g.drawRoundedRectangle (r, 14.0f, 1.0f);

    const auto& info = modules[module];
    g.setColour (text);
    g.setFont (swellFont (20.0f, true));
    g.drawText (info.name, 138, 18, 120, 36, juce::Justification::centredLeft);
    g.setColour (dim);
    g.setFont (swellFont (12.0f));
    g.drawText (utf8 (info.hint), 250, 18, 300, 36, juce::Justification::centredLeft);

    drawSectionLabel (g, "MODE", { 22, 72, 200, 16 });

    const int colW = (getWidth() - 44 - 48) / 3;
    for (int k = 0; k < 3; ++k)
    {
        const juce::Rectangle<int> row (22 + k * (colW + 24), 172, colW, 18);
        g.setColour (dim);
        g.setFont (swellFont (12.0f));
        g.drawText (utf8 (info.labels[k]), row, juce::Justification::centredLeft);
        g.setColour (orange);
        g.setFont (swellFont (13.0f, true));
        g.drawText (sliders[k].getTextFromValue (sliders[k].getValue()), row, juce::Justification::centredRight);
    }
}

//==============================================================================
void PresetList::setCategory (const juce::String& cat)
{
    items.clear();
    const auto& list = Presets::all();
    for (int i = 0; i < (int) list.size(); ++i)
        if (cat == list[(size_t) i].category)
            items.push_back (i);
    repaint();
}

void PresetList::paint (juce::Graphics& g)
{
    using namespace SwellColours;
    const auto& list = Presets::all();
    const int current = proc.getCurrentPreset();

    for (int row = 0; row < (int) items.size(); ++row)
    {
        const auto r = juce::Rectangle<int> (0, row * (rowH + 4), getWidth(), rowH).toFloat();
        const bool sel = items[(size_t) row] == current;
        if (sel || row == hover)
        {
            g.setColour (sel ? juce::Colour (0xff2e2620) : control);
            g.fillRoundedRectangle (r, 8.0f);
        }
        g.setColour (sel ? orange : text);
        g.setFont (swellFont (14.0f, sel));
        g.drawText (list[(size_t) items[(size_t) row]].name, r.reduced (12.0f, 0.0f), juce::Justification::centredLeft);
        if (sel)
        {
            g.setColour (orange);
            g.fillEllipse (r.getRight() - 18.0f, r.getCentreY() - 3.0f, 6.0f, 6.0f);
        }
    }
}

void PresetList::mouseMove (const juce::MouseEvent& e)
{
    const int row = e.y / (rowH + 4);
    const int h = row < (int) items.size() ? row : -1;
    if (h != hover) { hover = h; repaint(); }
}

void PresetList::mouseUp (const juce::MouseEvent& e)
{
    const int row = e.y / (rowH + 4);
    if (e.mouseWasClicked() && juce::isPositiveAndBelow (row, (int) items.size()))
    {
        proc.loadPreset (items[(size_t) row]);
        repaint();
    }
}

//==============================================================================
void PresetDisplay::paint (juce::Graphics& g)
{
    using namespace SwellColours;
    const auto& p = Presets::all()[(size_t) proc.getCurrentPreset()];
    g.setColour (dim);
    g.setFont (swellFont (10.5f, true));
    g.drawText (juce::String (p.category).toUpperCase(), getLocalBounds().withHeight (16).withY (3), juce::Justification::centred);
    g.setColour (text);
    g.setFont (swellFont (17.0f, true));
    g.drawText (p.name, getLocalBounds().withTop (17).withHeight (22), juce::Justification::centred);
}

void PresetDisplay::mouseUp (const juce::MouseEvent& e)
{
    if (! e.mouseWasClicked())
        return;

    juce::PopupMenu menu;
    const auto& list = Presets::all();
    for (auto* cat : Presets::categories)
    {
        menu.addSectionHeader (juce::String (cat).toUpperCase());
        for (int i = 0; i < (int) list.size(); ++i)
            if (juce::String (cat) == list[(size_t) i].category)
                menu.addItem (i + 1, list[(size_t) i].name, true, i == proc.getCurrentPreset());
    }

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [this] (int result) { if (result > 0 && onPick) onPick (result - 1); });
}

//==============================================================================
void ThrowButton::paintButton (juce::Graphics& g, bool over, bool)
{
    using namespace SwellColours;
    const auto r = getLocalBounds().toFloat().reduced (1.0f);
    const bool on = getToggleState();
    g.setColour (on ? orange : (over ? orange.withAlpha (0.08f) : juce::Colours::transparentBlack));
    g.fillRoundedRectangle (r, 12.0f);
    g.setColour (orange);
    g.drawRoundedRectangle (r, 12.0f, 2.0f);

    g.setColour (on ? background : orange);
    g.setFont (swellFont (18.0f, true));
    g.drawText ("THROW", r.withHeight (r.getHeight() * 0.62f), juce::Justification::centredBottom);
    g.setFont (swellFont (10.0f));
    g.drawText ("delay fin de mot", r.withTop (r.getY() + r.getHeight() * 0.6f), juce::Justification::centredTop);
}

void Meter::paint (juce::Graphics& g)
{
    using namespace SwellColours;
    const auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (well);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (border);
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    const float db = juce::Decibels::gainToDecibels (level, -60.0f);
    const float h = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f) * (r.getHeight() - 2.0f);
    const auto bar = r.reduced (1.0f).withTop (r.getBottom() - 1.0f - h);
    g.setGradientFill (juce::ColourGradient (orange, 0.0f, r.getBottom(), juce::Colour (0xffffd27a), 0.0f, r.getY(), false));
    g.fillRoundedRectangle (bar, 5.0f);
}

//==============================================================================
SwellAdLibEditor::SwellAdLibEditor (SwellAdLibProcessor& p)
    : AudioProcessorEditor (&p), proc (p), presetDisplay (p), presetList (p),
      live (p), macros (p), modulePanel (p)
{
    setLookAndFeel (&lnf);

    // Header
    prevButton.onClick = [this] { const int n = (int) Presets::all().size(); proc.loadPreset ((proc.getCurrentPreset() + n - 1) % n); };
    nextButton.onClick = [this] { proc.loadPreset ((proc.getCurrentPreset() + 1) % (int) Presets::all().size()); };
    presetDisplay.onPick = [this] (int i) { proc.loadPreset (i); };
    presetDisplay.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    abButton.onClick = [this] { proc.toggleAB(); };
    abButton.setTooltip ("Compare deux reglages (A / B)");
    bypassButton.setClickingTogglesState (true);
    bypassAtt = std::make_unique<ButtonAttachment> (proc.apvts, "bypass", bypassButton);
    for (auto* c : std::initializer_list<juce::Component*> { &prevButton, &nextButton, &presetDisplay, &abButton, &bypassButton })
        addAndMakeVisible (c);

    // Preset browser
    for (auto* cat : Presets::categories)
    {
        auto* b = categoryButtons.add (new juce::TextButton (cat));
        b->onClick = [this, name = juce::String (cat)] { selectCategory (name); };
        addAndMakeVisible (b);
    }
    addAndMakeVisible (presetList);

    // Center
    addAndMakeVisible (live);
    addAndMakeVisible (macros);
    addChildComponent (modulePanel);
    modulePanel.onBack = [this] { selectModule (-1); };
    for (int i = 0; i < 7; ++i)
    {
        auto* b = moduleButtons.add (new ModuleButton (modules[i].name, modules[i].icon));
        b->onClick = [this, i] { selectModule (selected == i ? -1 : i); };
        addAndMakeVisible (b);
    }

    // Output
    throwAtt = std::make_unique<ButtonAttachment> (proc.apvts, "throw", throwButton);
    throwButton.setTooltip ("Le delay ne part que sur la fin des mots");
    addAndMakeVisible (throwButton);
    addAndMakeVisible (meterL);
    addAndMakeVisible (meterR);
    for (auto* s : { &mixSlider, &gainSlider })
    {
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s->onValueChange = [this] { repaint (952, 430, 168, 140); };
        addAndMakeVisible (s);
    }
    mixAtt = std::make_unique<SliderAttachment> (proc.apvts, "out_mix", mixSlider);
    gainAtt = std::make_unique<SliderAttachment> (proc.apvts, "out_gain", gainSlider);

    setSize (1120, 720);
    presetChanged();
    startTimerHz (30);
}

SwellAdLibEditor::~SwellAdLibEditor()
{
    setLookAndFeel (nullptr);
}

void SwellAdLibEditor::selectCategory (const juce::String& cat)
{
    category = cat;
    for (auto* b : categoryButtons)
        b->setToggleState (b->getButtonText() == cat, juce::dontSendNotification);
    presetList.setCategory (cat);
}

void SwellAdLibEditor::selectModule (int index)
{
    selected = index;
    if (index >= 0)
        modulePanel.setModule (index);
    modulePanel.setVisible (index >= 0);
    macros.setVisible (index < 0);
    timerCallback();
}

void SwellAdLibEditor::presetChanged()
{
    shownPreset = proc.getCurrentPreset();
    selectCategory (Presets::all()[(size_t) shownPreset].category);
    presetDisplay.repaint();
    repaint (0, 64, 236, 626);
}

void SwellAdLibEditor::timerCallback()
{
    if (proc.getCurrentPreset() != shownPreset)
        presetChanged();

    for (int i = 0; i < moduleButtons.size(); ++i)
    {
        moduleButtons[i]->setState (paramOn (proc, modules[i].onId), i == selected);
        if (i == selected || moduleButtons[i]->getTooltip().isEmpty())
            moduleButtons[i]->setTooltip (juce::String (modules[i].name) + " : " + utf8 (modules[i].hint));
    }
    if (modulePanel.isVisible())
        modulePanel.refresh();

    abButton.setButtonText (proc.isOnB() ? "B" : "A");

    const float alpha = paramOn (proc, "bypass") ? 0.35f : 1.0f;
    for (auto* c : std::initializer_list<juce::Component*> { &presetList, &live, &macros, &modulePanel, &throwButton, &mixSlider, &gainSlider })
        c->setAlpha (alpha);
    for (auto* b : moduleButtons) b->setAlpha (alpha);

    // Dev helper for the README pictures: SWELL_SNAPSHOT=<file.png> [SWELL_SNAPSHOT_PRESET=n] [SWELL_SNAPSHOT_MODULE=n]
    if (! snapshotDone)
    {
        snapshotDone = true;
        const auto file = juce::SystemStats::getEnvironmentVariable ("SWELL_SNAPSHOT", {});
        if (file.isNotEmpty())
        {
            proc.loadPreset (juce::SystemStats::getEnvironmentVariable ("SWELL_SNAPSHOT_PRESET", "0").getIntValue());
            presetChanged();
            const int module = juce::SystemStats::getEnvironmentVariable ("SWELL_SNAPSHOT_MODULE", "-1").getIntValue();
            if (module >= 0)
                selectModule (module);
            for (int i = 0; i < SwellAdLibProcessor::waveSize; ++i)
                proc.wave[(size_t) i].store (0.04f + 0.3f * std::pow (std::abs (std::sin ((float) i * 0.19f) * std::sin ((float) i * 0.047f)), 1.5f));
            proc.peakL.store (0.5f);
            proc.peakR.store (0.45f);
            levelL = levelR = 0.0f;

            juce::Timer::callAfterDelay (300, [this, file]
            {
                juce::FileOutputStream out { juce::File (file) };
                if (out.openedOk())
                {
                    out.setPosition (0);
                    out.truncate();
                    juce::PNGImageFormat().writeImageToStream (createComponentSnapshot (getLocalBounds(), true, 2.0f), out);
                }
                if (auto* app = juce::JUCEApplicationBase::getInstance())
                    app->systemRequestedQuit();
            });
        }
    }

    levelL = juce::jmax (proc.peakL.exchange (0.0f), levelL * 0.85f);
    levelR = juce::jmax (proc.peakR.exchange (0.0f), levelR * 0.85f);
    meterL.setLevel (levelL);
    meterR.setLevel (levelR);
    live.repaint();
}

void SwellAdLibEditor::resized()
{
    // Header
    prevButton.setBounds (370, 12, 40, 40);
    presetDisplay.setBounds (414, 12, 300, 40);
    nextButton.setBounds (718, 12, 40, 40);
    bypassButton.setBounds (1020, 12, 80, 40);
    abButton.setBounds (968, 12, 44, 40);

    // Preset browser
    for (int i = 0; i < categoryButtons.size(); ++i)
        categoryButtons[i]->setBounds (16 + (i % 2) * 105, 104 + (i / 2) * 40, 99, 34);
    presetList.setBounds (16, 196, 204, 380);

    // Center
    live.setBounds (260, 84, 668, 92);
    macros.setBounds (260, 194, 668, 262);
    modulePanel.setBounds (260, 194, 668, 262);
    const float w = (668.0f - 6 * 8.0f) / 7.0f;
    for (int i = 0; i < moduleButtons.size(); ++i)
        moduleButtons[i]->setBounds (juce::roundToInt (260.0f + (float) i * (w + 8.0f)), 500, juce::roundToInt (w), 100);

    // Output
    throwButton.setBounds (968, 108, 136, 72);
    meterL.setBounds (1012, 200, 14, 216);
    meterR.setBounds (1046, 200, 14, 216);
    mixSlider.setBounds (968, 470, 136, 24);
    gainSlider.setBounds (968, 530, 136, 24);
}

void SwellAdLibEditor::paint (juce::Graphics& g)
{
    using namespace SwellColours;
    g.fillAll (background);

    // Header
    g.setColour (juce::Colour (0xff1d1d21));
    g.fillRect (0, 0, 1120, 64);
    g.setColour (border);
    g.drawHorizontalLine (64, 0.0f, 1120.0f);
    drawSwellLogo (g, { 18.0f, 14.0f, 36.0f, 36.0f });
    g.setColour (text);
    g.setFont (swellFont (18.0f, true));
    g.drawText ("SWELL", 64, 14, 150, 20, juce::Justification::centredLeft);
    g.setColour (orange);
    g.setFont (swellFont (12.0f, true));
    g.drawText ("AD-LIB FX", 64, 34, 150, 16, juce::Justification::centredLeft);

    g.setColour (well);
    g.fillRoundedRectangle (366.0f, 8.0f, 396.0f, 48.0f, 10.0f);
    g.setColour (border);
    g.drawRoundedRectangle (366.5f, 8.5f, 395.0f, 47.0f, 10.0f, 1.0f);

    // Side panels
    g.setColour (panelDark);
    g.fillRect (0, 65, 236, 625);
    g.fillRect (952, 65, 168, 625);
    g.setColour (border);
    g.drawVerticalLine (236, 65.0f, 690.0f);
    g.drawVerticalLine (952, 65.0f, 690.0f);

    // Preset browser
    drawSectionLabel (g, "PRESETS", { 16, 80, 200, 16 });
    const auto& preset = Presets::all()[(size_t) proc.getCurrentPreset()];
    const juce::Rectangle<float> desc (16.0f, 590.0f, 204.0f, 84.0f);
    g.setColour (well);
    g.fillRoundedRectangle (desc, 10.0f);
    g.setColour (border);
    g.drawRoundedRectangle (desc, 10.0f, 1.0f);
    g.setColour (dim);
    g.setFont (swellFont (12.0f));
    g.drawFittedText (preset.description, desc.reduced (12.0f, 10.0f).toNearestInt(), juce::Justification::topLeft, 5, 1.0f);

    // Chain
    drawSectionLabel (g, utf8 ("CHA\xc3\x8eNE D'EFFETS"), { 260, 474, 120, 16 });
    g.setColour (dim);
    g.setFont (swellFont (11.0f));
    g.drawText (utf8 ("\xc2\xb7 clique sur un effet pour le r\xc3\xa9gler"), 372, 474, 300, 16, juce::Justification::centredLeft);

    // Output
    drawSectionLabel (g, "SORTIE", { 968, 80, 120, 16 });
    g.setColour (dim);
    g.setFont (swellFont (10.0f));
    g.drawText ("L", 1012, 420, 14, 14, juce::Justification::centred);
    g.drawText ("R", 1046, 420, 14, 14, juce::Justification::centred);

    g.setFont (swellFont (12.0f));
    g.drawText ("MIX", 968, 448, 60, 18, juce::Justification::centredLeft);
    g.drawText ("GAIN", 968, 508, 60, 18, juce::Justification::centredLeft);
    g.setColour (text);
    g.drawText (mixSlider.getTextFromValue (mixSlider.getValue()), 1028, 448, 76, 18, juce::Justification::centredRight);
    g.drawText (gainSlider.getTextFromValue (gainSlider.getValue()), 1028, 508, 76, 18, juce::Justification::centredRight);

    // Footer
    g.setColour (well);
    g.fillRect (0, 690, 1120, 30);
    g.setColour (border);
    g.drawHorizontalLine (690, 0.0f, 1120.0f);
    g.setColour (dim);
    g.setFont (swellFont (11.0f));
    g.drawText ("Swell Ad-lib FX v" + juce::String (JucePlugin_VersionString), 20, 690, 300, 30, juce::Justification::centredLeft);
    g.drawText ("by @sevyondatracks", 800, 690, 300, 30, juce::Justification::centredRight);
}
