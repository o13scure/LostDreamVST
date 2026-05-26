/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class SaturatorLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SaturatorLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;
};

//==============================================================================
class SaturatorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SaturatorAudioProcessorEditor(SaturatorAudioProcessor&);
    ~SaturatorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void configureRotaryKnob(juce::Slider& slider, bool largeKnob);
    void configureVerticalSlider(juce::Slider& slider);
    void drawSection(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title) const;
    void layoutSquareRotary(juce::Rectangle<int> area, juce::Slider& slider);
    void layoutTopPowerButton(juce::Rectangle<int> topRow, juce::ToggleButton& button);
    void updatePowerButtonText(juce::ToggleButton& button);
    void setupPowerButton(juce::ToggleButton& button);
    void updateGateThresholdLabel();

    SaturatorLookAndFeel pluginLookAndFeel;
    SaturatorAudioProcessor& audioProcessor;

    // Gate
    juce::ToggleButton gateOnButton;
    juce::Slider gateThresholdSlider;
    juce::Label gateThresholdLabel;
    juce::Label gateThresholdValueLabel;

    // Reverb
    juce::ToggleButton reverbOnButton;
    juce::Slider roomSizeSlider;
    juce::Slider dampingSlider;
    juce::Slider reverbMixSlider;
    juce::Slider widthSlider;
    juce::Label roomSizeLabel;
    juce::Label dampingLabel;
    juce::Label reverbMixLabel;
    juce::Label widthLabel;

    // Drive
    juce::ComboBox driveModeCombo;
    juce::Slider driveSlider;
    juce::Slider preGainSlider;
    juce::Slider postGainSlider;
    juce::Label driveLabel;
    juce::Label preGainLabel;
    juce::Label postGainLabel;

    // Mix / Output
    juce::Slider mixSlider;
    juce::Slider outputSlider;
    juce::Label mixLabel;
    juce::Label outputLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttachment> gateOnAttachment;
    std::unique_ptr<SliderAttachment> gateThresholdAttachment;

    std::unique_ptr<ButtonAttachment> reverbOnAttachment;
    std::unique_ptr<SliderAttachment> roomSizeAttachment;
    std::unique_ptr<SliderAttachment> dampingAttachment;
    std::unique_ptr<SliderAttachment> reverbMixAttachment;
    std::unique_ptr<SliderAttachment> widthAttachment;

    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<ComboBoxAttachment> driveModeAttachment;
    std::unique_ptr<SliderAttachment> preGainAttachment;
    std::unique_ptr<SliderAttachment> postGainAttachment;

    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturatorAudioProcessorEditor)
};
