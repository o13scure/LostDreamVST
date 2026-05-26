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

    constexpr int editorWidth  = 980;
    constexpr int editorHeight = 320;
    constexpr int topBarHeight = 32;
    constexpr int powerButtonW = 72;
    constexpr int powerButtonH = 28;
    constexpr int smallKnobSize = 52;
    constexpr int largeKnobSize = 108;
}

//==============================================================================
SaturatorLookAndFeel::SaturatorLookAndFeel()
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

void SaturatorLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
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

void SaturatorLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
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

void SaturatorLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
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
SaturatorAudioProcessorEditor::SaturatorAudioProcessorEditor(SaturatorAudioProcessor& p)
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

    configureRotaryKnob(roomSizeSlider, true);
    configureRotaryKnob(dampingSlider, false);
    configureRotaryKnob(reverbMixSlider, false);
    configureRotaryKnob(widthSlider, false);
    addAndMakeVisible(roomSizeSlider);
    addAndMakeVisible(dampingSlider);
    addAndMakeVisible(reverbMixSlider);
    addAndMakeVisible(widthSlider);
    setupLabel(roomSizeLabel, "ROOM");
    setupLabel(dampingLabel, "DAMP");
    setupLabel(reverbMixLabel, "MIX");
    setupLabel(widthLabel, "WIDTH");

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

    reverbOnAttachment = std::make_unique<ButtonAttachment>(apvts, "ReverbOn", reverbOnButton);
    roomSizeAttachment = std::make_unique<SliderAttachment>(apvts, "RoomSize", roomSizeSlider);
    dampingAttachment = std::make_unique<SliderAttachment>(apvts, "Damping", dampingSlider);
    reverbMixAttachment = std::make_unique<SliderAttachment>(apvts, "ReverbMix", reverbMixSlider);
    widthAttachment = std::make_unique<SliderAttachment>(apvts, "Width", widthSlider);

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

SaturatorAudioProcessorEditor::~SaturatorAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void SaturatorAudioProcessorEditor::configureRotaryKnob(juce::Slider& slider, bool largeKnob)
{
    slider.getProperties().set("knobSize", largeKnob ? "large" : "small");
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                               juce::MathConstants<float>::pi * 2.8f, true);
    slider.setMouseDragSensitivity(largeKnob ? 220 : 180);
}

void SaturatorAudioProcessorEditor::configureVerticalSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::LinearVertical);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled(true, true, this);
}

void SaturatorAudioProcessorEditor::layoutSquareRotary(juce::Rectangle<int> area, juce::Slider& slider)
{
    const auto size = juce::jmin(area.getWidth(), area.getHeight());
    slider.setBounds(area.withSizeKeepingCentre(size, size));
}

void SaturatorAudioProcessorEditor::layoutTopPowerButton(juce::Rectangle<int> topRow, juce::ToggleButton& button)
{
    button.setBounds(topRow.withSizeKeepingCentre(powerButtonW, powerButtonH));
}

void SaturatorAudioProcessorEditor::updatePowerButtonText(juce::ToggleButton& button)
{
    button.setButtonText(button.getToggleState() ? "ON" : "OFF");
}

void SaturatorAudioProcessorEditor::setupPowerButton(juce::ToggleButton& button)
{
    button.setClickingTogglesState(true);
    button.onClick = [this, &button] { updatePowerButtonText(button); };
    addAndMakeVisible(button);
}

void SaturatorAudioProcessorEditor::updateGateThresholdLabel()
{
    gateThresholdValueLabel.setText(
        juce::String(gateThresholdSlider.getValue(), 1) + " dB",
        juce::dontSendNotification);
}

void SaturatorAudioProcessorEditor::drawSection(juce::Graphics& g, juce::Rectangle<int> bounds,
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

void SaturatorAudioProcessorEditor::paint(juce::Graphics& g)
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
    const int sectionWidth = (bounds.getWidth() - gap * 3) / 4;

    auto gateArea   = bounds.removeFromLeft(sectionWidth);
    bounds.removeFromLeft(gap);
    auto reverbArea = bounds.removeFromLeft(sectionWidth);
    bounds.removeFromLeft(gap);
    auto driveArea  = bounds.removeFromLeft(sectionWidth);
    bounds.removeFromLeft(gap);
    auto mixArea    = bounds;

    drawSection(g, gateArea, {});
    drawSection(g, reverbArea, {});
    drawSection(g, driveArea, {});
    drawSection(g, mixArea, {});
}

void SaturatorAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    const int gap = 8;
    const int sectionWidth = (bounds.getWidth() - gap * 3) / 4;

    auto placeSection = [&](juce::Rectangle<int> area)
    {
        area.removeFromTop(28);
        return area.reduced(8, 4);
    };

    auto gateArea = placeSection(bounds.removeFromLeft(sectionWidth));
    bounds.removeFromLeft(gap);
    auto reverbArea = placeSection(bounds.removeFromLeft(sectionWidth));
    bounds.removeFromLeft(gap);
    auto driveArea = placeSection(bounds.removeFromLeft(sectionWidth));
    bounds.removeFromLeft(gap);
    auto mixArea = placeSection(bounds);

    auto layoutSmallKnob = [this](juce::Rectangle<int>& column, int knobSize,
                                  juce::Label& label, juce::Slider& slider)
    {
        auto cell = column.removeFromTop(14 + knobSize + 6);
        label.setBounds(cell.removeFromTop(14));
        layoutSquareRotary(cell, slider);
    };

    // --- Gate ---
    auto gateTop = gateArea.removeFromTop(topBarHeight);
    layoutTopPowerButton(gateTop, gateOnButton);

    auto threshColumn = gateArea.withSizeKeepingCentre(64, gateArea.getHeight());
    gateThresholdLabel.setBounds(threshColumn.removeFromTop(16));
    gateThresholdValueLabel.setBounds(threshColumn.removeFromBottom(18));
    gateThresholdSlider.setBounds(threshColumn.reduced(4, 2));

    // --- Reverb ---
    auto reverbTop = reverbArea.removeFromTop(topBarHeight);
    layoutTopPowerButton(reverbTop, reverbOnButton);

    auto reverbKnobs = reverbArea;
    auto reverbSmallCol = reverbKnobs.removeFromRight(smallKnobSize + 8);
    layoutSmallKnob(reverbSmallCol, smallKnobSize, dampingLabel, dampingSlider);
    layoutSmallKnob(reverbSmallCol, smallKnobSize, reverbMixLabel, reverbMixSlider);
    layoutSmallKnob(reverbSmallCol, smallKnobSize, widthLabel, widthSlider);

    auto roomCell = reverbKnobs;
    roomSizeLabel.setBounds(roomCell.removeFromTop(14));
    layoutSquareRotary(roomCell.withSizeKeepingCentre(largeKnobSize, largeKnobSize), roomSizeSlider);

    // --- Drive ---
    auto driveTop = driveArea.removeFromTop(topBarHeight);
    driveModeCombo.setBounds(driveTop.withSizeKeepingCentre(96, powerButtonH));

    auto driveKnobs = driveArea;
    auto driveSmallCol = driveKnobs.removeFromRight(smallKnobSize + 8);
    layoutSmallKnob(driveSmallCol, smallKnobSize, postGainLabel, postGainSlider);
    layoutSmallKnob(driveSmallCol, smallKnobSize, preGainLabel, preGainSlider);

    auto driveMain = driveKnobs;
    driveLabel.setBounds(driveMain.removeFromTop(14));
    layoutSquareRotary(driveMain.withSizeKeepingCentre(largeKnobSize + 8, largeKnobSize + 8), driveSlider);

    // --- Mix / Output ---
    const int mixKnob = 72;
    auto mixCell = mixArea.removeFromTop(mixArea.getHeight() / 2 - 2);
    auto outCell = mixArea;

    mixLabel.setBounds(mixCell.removeFromTop(14));
    layoutSquareRotary(mixCell.withSizeKeepingCentre(mixKnob, mixKnob), mixSlider);

    outputLabel.setBounds(outCell.removeFromTop(14));
    layoutSquareRotary(outCell.withSizeKeepingCentre(mixKnob, mixKnob), outputSlider);
}
