/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"

namespace
{
    const auto kBackground   = juce::Colour(0xff070707);
    const auto kPanelFill    = juce::Colour(0xff111214);
    const auto kAccent       = juce::Colour(0xffad2dd4);
    const auto kAccentDim    = juce::Colour(0xff870ea5);
    const auto kKnobHighlight = juce::Colour(0xfff4f6f8);
    const auto kKnobMid       = juce::Colour(0xffc4c9cf);
    const auto kKnobShadow    = juce::Colour(0xff6d737a);
    const auto kKnobRim       = juce::Colour(0xff3f444a);
    const auto kText         = juce::Colour(0xfff0f0f0);
    const auto kTextDim      = juce::Colour(0xffa9b0b4);

    constexpr int editorWidth  = 1200;
    constexpr int editorHeight = 400;
    constexpr int numSections  = 5;
    constexpr int topBarHeight = 32;
    constexpr int powerButtonW = 72;
    constexpr int powerButtonH = 28;
    constexpr int labelHeight = 14;
    constexpr int knobGap = 4;
    constexpr int sectionPad = 8;
}

//==============================================================================
LostDreamLookAndFeel::LostDreamLookAndFeel()
{
    setColour(juce::Slider::thumbColourId, kKnobMid);
    setColour(juce::Slider::rotarySliderFillColourId, kAccent);
    setColour(juce::Slider::rotarySliderOutlineColourId, kKnobRim);
    setColour(juce::Slider::trackColourId, kAccent);
    setColour(juce::Slider::backgroundColourId, juce::Colour(0xff1e1e1e));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1e1e1e));
    setColour(juce::ComboBox::outlineColourId, kAccentDim);
    setColour(juce::ComboBox::textColourId, kText);
    setColour(juce::ToggleButton::textColourId, kText);
    setColour(juce::ToggleButton::tickColourId, kAccent);
    setColour(juce::ToggleButton::tickDisabledColourId, kKnobRim);
    setColour(juce::Label::textColourId, kTextDim);
}

void LostDreamLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPosProportional, float rotaryStartAngle,
                                            float rotaryEndAngle, juce::Slider& slider)
{
    const auto diameter = (float) juce::jmin(width, height);
    const auto bounds = juce::Rectangle<float>(x + (width - (int) diameter) * 0.5f,
                                               y + (height - (int) diameter) * 0.5f,
                                               diameter, diameter).reduced(1.5f);
    const auto radius = bounds.getWidth() * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const bool isLarge = slider.getProperties().getWithDefault("knobSize", "small") == juce::String("large");

    // Drop shadow (reference-style floating knob)
    {
        auto shadow = bounds.translated(0.0f, radius * 0.07f).expanded(radius * 0.03f);
        g.setColour(juce::Colours::black.withAlpha(0.42f));
        g.fillEllipse(shadow);
        g.setColour(juce::Colours::black.withAlpha(0.18f));
        g.fillEllipse(shadow.expanded(radius * 0.06f));
    }

    // Brushed-metal dome (radial highlight)
    juce::ColourGradient dome(
        kKnobHighlight, centre.x - radius * 0.42f, centre.y - radius * 0.48f,
        kKnobShadow,    centre.x + radius * 0.38f, centre.y + radius * 0.42f,
        true);
    dome.addColour(0.42f, kKnobMid.brighter(0.08f));
    dome.addColour(0.72f, kKnobMid.darker(0.06f));
    g.setGradientFill(dome);
    g.fillEllipse(bounds);

    // Diagonal brushed streaks
    juce::ColourGradient streak(
        juce::Colours::white.withAlpha(0.20f), bounds.getX(), bounds.getY(),
        juce::Colours::black.withAlpha(0.14f), bounds.getRight(), bounds.getBottom(),
        false);
    g.setGradientFill(streak);
    g.fillEllipse(bounds.reduced(radius * 0.06f));

    // Specular crescent (top-left shine)
    juce::Path spec;
    spec.addCentredArc(centre.x - radius * 0.12f, centre.y - radius * 0.18f,
                        radius * 0.72f, radius * 0.55f, 0.0f,
                        juce::MathConstants<float>::pi * 1.05f,
                        juce::MathConstants<float>::pi * 1.85f, true);
    g.setColour(juce::Colours::white.withAlpha(0.16f));
    g.fillPath(spec);

    // Outer bezel
    g.setColour(kKnobRim.withAlpha(0.95f));
    g.drawEllipse(bounds, 1.15f);

    // Inner rim highlight
    g.setColour(juce::Colours::white.withAlpha(0.22f));
    g.drawEllipse(bounds.reduced(radius * 0.11f), 0.75f);

    // Subtle bottom edge shade
    juce::Path shade;
    shade.addCentredArc(centre.x, centre.y + radius * 0.04f,
                        radius * 0.86f, radius * 0.62f, 0.0f,
                        juce::MathConstants<float>::pi * 0.12f,
                        juce::MathConstants<float>::pi * 0.88f, true);
    g.setColour(juce::Colours::black.withAlpha(0.10f));
    g.fillPath(shade);

    // Thin black indicator line (reference style)
    const float lineInner = radius * (isLarge ? 0.20f : 0.22f);
    const float lineOuter = radius * (isLarge ? 0.76f : 0.72f);
    const float dirX = std::sin(angle);
    const float dirY = -std::cos(angle);
    const float lineW = isLarge ? 1.55f : 1.15f;

    g.setColour(juce::Colours::black.withAlpha(0.88f));
    g.drawLine(centre.x + dirX * lineInner, centre.y + dirY * lineInner,
               centre.x + dirX * lineOuter, centre.y + dirY * lineOuter, lineW);

    // Micro highlight on indicator tip
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.drawLine(centre.x + dirX * (lineOuter - 2.0f), centre.y + dirY * (lineOuter - 2.0f),
               centre.x + dirX * lineOuter, centre.y + dirY * lineOuter, 0.6f);
}

void LostDreamLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                            float, float, float,
                                            juce::Slider::SliderStyle, juce::Slider& slider)
{
    const auto track = juce::Rectangle<float>(static_cast<float>(x) + width * 0.4f,
                                              static_cast<float>(y) + 6.0f,
                                              static_cast<float>(width) * 0.2f,
                                              static_cast<float>(height) - 12.0f);

    juce::ColourGradient trackGrad(
        juce::Colour(0xff2a2c30), track.getX(), track.getY(),
        juce::Colour(0xff101114), track.getX(), track.getBottom(),
        false);
    g.setGradientFill(trackGrad);
    g.fillRoundedRectangle(track, track.getWidth() * 0.5f);

    const auto fillHeight = track.getHeight() * slider.valueToProportionOfLength(slider.getValue());
    auto fill = track.withTrimmedTop(track.getHeight() - fillHeight);

    g.setColour(kAccent.withAlpha(0.85f));
    g.fillRoundedRectangle(fill, track.getWidth() * 0.5f);

    const auto thumbY = juce::jmap(static_cast<float>(slider.getValue()),
                                   static_cast<float>(slider.getMinimum()),
                                   static_cast<float>(slider.getMaximum()),
                                   track.getBottom() - 7.0f, track.getY() + 7.0f);

    g.setColour(kKnobMid.brighter(0.1f));
    g.fillEllipse(track.getCentreX() - 7.0f, thumbY - 7.0f, 14.0f, 14.0f);
    g.setColour(kAccent);
    g.drawEllipse(track.getCentreX() - 7.0f, thumbY - 7.0f, 14.0f, 14.0f, 1.2f);
}

void LostDreamLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    const auto isOn = button.getToggleState();
    if (isOn)
    {
        g.setColour(kAccent);
        g.fillRoundedRectangle(bounds, 6.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xff0f1012));
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(kAccentDim.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds, 6.0f, 1.2f);
    }

    if (shouldDrawButtonAsHighlighted)
    {
        g.setColour(kAccent.withAlpha(0.26f));
        g.fillRoundedRectangle(bounds, 6.0f);
    }

    if (shouldDrawButtonAsDown)
    {
        g.setColour(kAccentDim.withAlpha(0.65f));
        g.fillRoundedRectangle(bounds, 6.0f);
    }

    const auto textColour = isOn ? juce::Colours::black : kTextDim;
    g.setColour(textColour);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
}

//==============================================================================
LostDreamAudioProcessorEditor::LostDreamAudioProcessorEditor(LostDreamAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p)
{
    setLookAndFeel(&pluginLookAndFeel);

    auto setupLabel = [this](juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setInterceptsMouseClicks(false, false);
        label.setFont(juce::FontOptions(11.0f));
        label.setColour(juce::Label::textColourId, kTextDim.withAlpha(0.65f));
        addAndMakeVisible(label);
    };

    setupPowerButton(gateOnButton);
    setupPowerButton(reverbOnButton);

    configureVerticalSlider(gateThresholdSlider);
    gateThresholdSlider.onValueChange = [this] { updateGateThresholdLabel(); };
    addAndMakeVisible(gateThresholdSlider);
    setupLabel(gateThresholdLabel, "THRESH");
    setupLabel(gateThresholdValueLabel, "-40.0 dB");
    gateThresholdValueLabel.setColour(juce::Label::textColourId, kText);
    gateThresholdValueLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));

    configureRotaryKnob(gateAttackSlider, false);
    configureRotaryKnob(gateReleaseSlider, false);
    addAndMakeVisible(gateAttackSlider);
    addAndMakeVisible(gateReleaseSlider);
    setupLabel(gateAttackLabel, "ATK");
    setupLabel(gateReleaseLabel, "REL");

    configureRotaryKnob(roomSizeSlider, true);
    configureRotaryKnob(dampingSlider, false);
    configureRotaryKnob(reverbMixSlider, false);
    configureRotaryKnob(reverbDecaySlider, false);
    configureRotaryKnob(reverbDiffusionSlider, false);
    configureRotaryKnob(reverbPreDelaySlider, false);
    configureRotaryKnob(reverbLowCutSlider, false);
    addAndMakeVisible(roomSizeSlider);
    addAndMakeVisible(dampingSlider);
    addAndMakeVisible(reverbMixSlider);
    addAndMakeVisible(reverbDecaySlider);
    addAndMakeVisible(reverbDiffusionSlider);
    addAndMakeVisible(reverbPreDelaySlider);
    addAndMakeVisible(reverbLowCutSlider);
    
    setupLabel(roomSizeLabel, "SIZE");
    setupLabel(dampingLabel, "DAMP");
    setupLabel(reverbMixLabel, "MIX");
    setupLabel(reverbDecayLabel, "DEC");
    setupLabel(reverbDiffusionLabel, "DIFF");
    setupLabel(reverbPreDelayLabel, "PRE");
    setupLabel(reverbLowCutLabel, "LOW");
    

    crushTypeCombo.addItemList({ "Full", "Quiet", "Loud" }, 1);
    crushTypeCombo.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(crushTypeCombo);

    configureRotaryKnob(crushMixSlider, false);
    addAndMakeVisible(crushMixSlider);
    setupLabel(crushMixLabel, "MIX");

    driveModeCombo.addItemList({ "Soft", "Hard", "Fold" }, 1);
    driveModeCombo.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(driveModeCombo);

    configureRotaryKnob(driveSlider, true);
    configureRotaryKnob(preGainSlider, false);
    configureRotaryKnob(postGainSlider, false);
    addAndMakeVisible(driveSlider);
    addAndMakeVisible(preGainSlider);
    addAndMakeVisible(postGainSlider);
    setupLabel(driveLabel, "DRIVE");
    setupLabel(preGainLabel, "PRE");
    setupLabel(postGainLabel, "POST");

    configureRotaryKnob(mixSlider, false);
    configureRotaryKnob(outputSlider, false);
    addAndMakeVisible(mixSlider);
    addAndMakeVisible(outputSlider);
    setupLabel(mixLabel, "MIX");
    setupLabel(outputLabel, "OUT");

    auto& apvts = audioProcessor.apvts;

    gateOnAttachment = std::make_unique<ButtonAttachment>(apvts, "GateOn", gateOnButton);
    gateThresholdAttachment = std::make_unique<SliderAttachment>(apvts, "GateThreshold", gateThresholdSlider);
    gateAttackAttachment = std::make_unique<SliderAttachment>(apvts, "GateAttack", gateAttackSlider);
    gateReleaseAttachment = std::make_unique<SliderAttachment>(apvts, "GateRelease", gateReleaseSlider);

    reverbOnAttachment = std::make_unique<ButtonAttachment>(apvts, "ReverbOn", reverbOnButton);
    roomSizeAttachment = std::make_unique<SliderAttachment>(apvts, "RoomSize", roomSizeSlider);
    dampingAttachment = std::make_unique<SliderAttachment>(apvts, "Damping", dampingSlider);
    reverbMixAttachment = std::make_unique<SliderAttachment>(apvts, "ReverbMix", reverbMixSlider);
    reverbDecayAttachment = std::make_unique<SliderAttachment>(apvts, "ReverbDecay", reverbDecaySlider);
    reverbDiffusionAttachment = std::make_unique<SliderAttachment>(apvts, "ReverbDiffusion", reverbDiffusionSlider);
    reverbPreDelayAttachment = std::make_unique<SliderAttachment>(apvts, "ReverbPreDelay", reverbPreDelaySlider);
    reverbLowCutAttachment = std::make_unique<SliderAttachment>(apvts, "ReverbLowCut", reverbLowCutSlider);

    crushTypeAttachment = std::make_unique<ComboBoxAttachment>(apvts, "CrushType", crushTypeCombo);
    crushMixAttachment = std::make_unique<SliderAttachment>(apvts, "CrushMix", crushMixSlider);

    driveAttachment = std::make_unique<SliderAttachment>(apvts, "Drive", driveSlider);
    driveModeAttachment = std::make_unique<ComboBoxAttachment>(apvts, "DriveMode", driveModeCombo);
    preGainAttachment = std::make_unique<SliderAttachment>(apvts, "PreGain", preGainSlider);
    postGainAttachment = std::make_unique<SliderAttachment>(apvts, "PostGain", postGainSlider);

    mixAttachment = std::make_unique<SliderAttachment>(apvts, "Mix", mixSlider);
    outputAttachment = std::make_unique<SliderAttachment>(apvts, "Output", outputSlider);

    updatePowerButtonText(gateOnButton);
    updatePowerButtonText(reverbOnButton);
    updateGateThresholdLabel();

    setSize(editorWidth, editorHeight);
}

LostDreamAudioProcessorEditor::~LostDreamAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void LostDreamAudioProcessorEditor::configureRotaryKnob(juce::Slider& slider, bool largeKnob)
{
    slider.getProperties().set("knobSize", largeKnob ? "large" : "small");
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                               juce::MathConstants<float>::pi * 2.8f, true);
    slider.setMouseDragSensitivity(largeKnob ? 220 : 180);
}

void LostDreamAudioProcessorEditor::configureVerticalSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::LinearVertical);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled(true, true, this);
}

void LostDreamAudioProcessorEditor::layoutSquareRotary(juce::Rectangle<int> area, juce::Slider& slider)
{
    const auto size = juce::jmin(area.getWidth(), area.getHeight());
    slider.setBounds(area.withSizeKeepingCentre(size, size));
}

void LostDreamAudioProcessorEditor::layoutKnobInCell(juce::Rectangle<int> cell,
                                                     juce::Label& label, juce::Slider& slider,
                                                     int fixedKnobSize)
{
    label.setBounds(cell.removeFromTop(labelHeight));

    if (fixedKnobSize > 0)
        layoutSquareRotary(cell.withSizeKeepingCentre(fixedKnobSize, fixedKnobSize), slider);
    else
        layoutSquareRotary(cell, slider);
}

void LostDreamAudioProcessorEditor::layoutKnobColumnEven(
    juce::Rectangle<int> column,
    std::initializer_list<std::pair<juce::Label*, juce::Slider*>> knobs)
{
    const int count = static_cast<int>(knobs.size());
    if (count == 0)
        return;

    const int totalGap = knobGap * (count - 1);
    const int cellHeight = (column.getHeight() - totalGap) / count;

    int index = 0;
    for (const auto& knob : knobs)
    {
        auto cell = column.removeFromTop(cellHeight);
        layoutKnobInCell(cell, *knob.first, *knob.second);

        if (index < count - 1)
            column.removeFromTop(knobGap);

        ++index;
    }
}

void LostDreamAudioProcessorEditor::layoutKnobGridEven(
    juce::Rectangle<int> area, int cols, int rows,
    std::initializer_list<std::pair<juce::Label*, juce::Slider*>> knobs,
    bool fillColumnsFirst)
{
    const int count = static_cast<int>(knobs.size());
    if (count == 0 || cols <= 0 || rows <= 0)
        return;

    const int colGap = knobGap;
    const int rowGap = knobGap;
    const int colWidth = (area.getWidth() - colGap * (cols - 1)) / cols;
    const int rowHeight = (area.getHeight() - rowGap * (rows - 1)) / rows;

    int index = 0;
    for (const auto& knob : knobs)
    {
        int col, row;
        if (fillColumnsFirst)
        {
            col = index / rows;
            row = index % rows;
        }
        else
        {
            col = index % cols;
            row = index / cols;
        }

        auto cell = juce::Rectangle<int>(
            area.getX() + col * (colWidth + colGap),
            area.getY() + row * (rowHeight + rowGap),
            colWidth, rowHeight);

        layoutKnobInCell(cell, *knob.first, *knob.second);
        ++index;
    }
}

void LostDreamAudioProcessorEditor::layoutTopPowerButton(juce::Rectangle<int> topRow, juce::ToggleButton& button)
{
    button.setBounds(topRow.withSizeKeepingCentre(powerButtonW, powerButtonH));
}

void LostDreamAudioProcessorEditor::updatePowerButtonText(juce::ToggleButton& button)
{
    button.setButtonText(button.getToggleState() ? "ON" : "OFF");
}

void LostDreamAudioProcessorEditor::setupPowerButton(juce::ToggleButton& button)
{
    button.setClickingTogglesState(true);
    button.onClick = [this, &button] { updatePowerButtonText(button); };
    addAndMakeVisible(button);
}

void LostDreamAudioProcessorEditor::updateGateThresholdLabel()
{
    gateThresholdValueLabel.setText(
        juce::String(gateThresholdSlider.getValue(), 1) + " dB",
        juce::dontSendNotification);
}

void LostDreamAudioProcessorEditor::drawSection(juce::Graphics& g, juce::Rectangle<int> bounds,
                                                const juce::String& title) const
{
    auto r = bounds.toFloat().reduced(1.0f);

    juce::ColourGradient panelGrad(
        kPanelFill.brighter(0.06f), r.getX(), r.getY(),
        kPanelFill.darker(0.18f), r.getX(), r.getBottom(),
        false);
    g.setGradientFill(panelGrad);
    g.fillRoundedRectangle(r, 8.0f);

    // subtle outline
    g.setColour(kAccentDim.withAlpha(0.75f));
    g.drawRoundedRectangle(r, 8.0f, 1.1f);

    // very subtle glow
    g.setColour(kAccent.withAlpha(0.12f));
    g.drawRoundedRectangle(r, 8.0f, 2.5f);

    if (!title.isEmpty())
    {
        g.setColour(kAccent.withAlpha(0.45f));
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText(title, r.removeFromTop(22).toNearestInt(), juce::Justification::centred);
    }
}

void LostDreamAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto r = getLocalBounds();
    juce::ColourGradient bgGrad(
        kBackground.brighter(0.08f), (float)r.getX(), (float)r.getY(),
        kBackground.darker(0.22f), (float)r.getX(), (float)r.getBottom(),
        false);
    g.setGradientFill(bgGrad);
    g.fillRect(r);

    auto bounds = getLocalBounds().reduced(10);
    const int gap = 8;
    const int sectionWidth = (bounds.getWidth() - gap * (numSections - 1)) / numSections;

    auto gateArea   = bounds.removeFromLeft(sectionWidth);
    bounds.removeFromLeft(gap);
    auto reverbArea = bounds.removeFromLeft(sectionWidth);
    bounds.removeFromLeft(gap);
    auto driveArea  = bounds.removeFromLeft(sectionWidth);
    bounds.removeFromLeft(gap);
    auto crushArea  = bounds.removeFromLeft(sectionWidth);
    bounds.removeFromLeft(gap);
    auto mixArea    = bounds;

    drawSection(g, gateArea, {});
    drawSection(g, reverbArea, {});
    drawSection(g, driveArea, {});
    drawSection(g, crushArea, {});
    drawSection(g, mixArea, {});
}

void LostDreamAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    const int gap = 8;
    const int sectionWidth = (bounds.getWidth() - gap * (numSections - 1)) / numSections;

    auto placeSection = [&](juce::Rectangle<int> area)
    {
        area.removeFromTop(28);
        return area.reduced(sectionPad, 4);
    };

    auto gateArea = placeSection(bounds.removeFromLeft(sectionWidth));
    bounds.removeFromLeft(gap);
    auto reverbArea = placeSection(bounds.removeFromLeft(sectionWidth));
    bounds.removeFromLeft(gap);
    auto driveArea = placeSection(bounds.removeFromLeft(sectionWidth));
    bounds.removeFromLeft(gap);
    auto crushArea = placeSection(bounds.removeFromLeft(sectionWidth));
    bounds.removeFromLeft(gap);
    auto mixArea = placeSection(bounds);

    auto smallColumnWidth = [](juce::Rectangle<int> content, int knobCount)
    {
        const int cellHeight = (content.getHeight() - knobGap * (knobCount - 1)) / knobCount;
        const int knobFromHeight = juce::jmax(0, cellHeight - labelHeight);
        const int maxColShare = content.getWidth() * 42 / 100;
        return juce::jmin(maxColShare, knobFromHeight + sectionPad);
    };

    const int sharedSmallColW = smallColumnWidth(gateArea, 2);
    const int sharedSmallCellH = (gateArea.getHeight() - knobGap) / 2;
    const int sharedSmallKnobSize = juce::jmin(sharedSmallColW - sectionPad,
                                               juce::jmax(0, sharedSmallCellH - labelHeight));

    const int sharedLargeKnobSize = juce::jmin(gateArea.getWidth() - sharedSmallColW - sectionPad,
                                                 juce::jmax(0, gateArea.getHeight() - labelHeight));

    // --- Gate ---
    auto gateTop = gateArea.removeFromTop(topBarHeight);
    layoutTopPowerButton(gateTop, gateOnButton);

    auto gateSmallCol = gateArea.removeFromRight(sharedSmallColW);
    layoutKnobColumnEven(gateSmallCol,
                         { { &gateAttackLabel,  &gateAttackSlider },
                           { &gateReleaseLabel, &gateReleaseSlider } });

    auto threshColumn = gateArea.reduced(juce::jmax(4, (gateArea.getWidth() - 56) / 2), 0);
    gateThresholdLabel.setBounds(threshColumn.removeFromTop(labelHeight + 2));
    gateThresholdValueLabel.setBounds(threshColumn.removeFromBottom(labelHeight + 4));
    gateThresholdSlider.setBounds(threshColumn.reduced(4, 2));

    // --- Reverb ---
    auto reverbTop = reverbArea.removeFromTop(topBarHeight);
    layoutTopPowerButton(reverbTop, reverbOnButton);

    constexpr int reverbSmallRows = 3;
    constexpr int reverbSmallCols = 2;
    const int reverbCellH = (reverbArea.getHeight() - knobGap * (reverbSmallRows - 1)) / reverbSmallRows;
    const int reverbKnobFromHeight = juce::jmax(0, reverbCellH - labelHeight);
    const int reverbGridW = reverbSmallCols * reverbKnobFromHeight + knobGap * (reverbSmallCols - 1) + sectionPad;
    auto reverbSmallGrid = reverbArea.removeFromRight(juce::jmin(reverbGridW, reverbArea.getWidth() * 58 / 100));

    layoutKnobGridEven(reverbSmallGrid, reverbSmallCols, reverbSmallRows,
                       { { &dampingLabel,        &dampingSlider },
                         { &reverbMixLabel,        &reverbMixSlider },
                         { &reverbDecayLabel,      &reverbDecaySlider },
                         { &reverbDiffusionLabel,  &reverbDiffusionSlider },
                         { &reverbPreDelayLabel,   &reverbPreDelaySlider },
                         { &reverbLowCutLabel,     &reverbLowCutSlider } },
                       true);

    layoutKnobInCell(reverbArea, roomSizeLabel, roomSizeSlider);

    // --- Drive ---
    auto driveTop = driveArea.removeFromTop(topBarHeight);
    driveModeCombo.setBounds(driveTop.withSizeKeepingCentre(juce::jmin(96, driveTop.getWidth() - 8), powerButtonH));

    auto driveSmallCol = driveArea.removeFromRight(sharedSmallColW);
    layoutKnobColumnEven(driveSmallCol,
                         { { &preGainLabel,  &preGainSlider },
                           { &postGainLabel, &postGainSlider } });

    layoutKnobInCell(driveArea, driveLabel, driveSlider);

    // --- Bit crush ---
    auto crushTop = crushArea.removeFromTop(topBarHeight);
    crushTypeCombo.setBounds(crushTop.withSizeKeepingCentre(juce::jmin(100, crushTop.getWidth() - 8), powerButtonH));
    layoutKnobInCell(crushArea, crushMixLabel, crushMixSlider, sharedLargeKnobSize);

    // --- Mix / Output ---
    const int mixRowGap = knobGap;
    const int mixRowH = (mixArea.getHeight() - mixRowGap) / 2;
    auto mixCell = mixArea.removeFromTop(mixRowH);
    mixArea.removeFromTop(mixRowGap);
    auto outCell = mixArea;

    layoutKnobInCell(mixCell, mixLabel, mixSlider, sharedSmallKnobSize);
    layoutKnobInCell(outCell, outputLabel, outputSlider, sharedSmallKnobSize);
}
