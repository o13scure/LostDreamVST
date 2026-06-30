/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class LostDreamLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LostDreamLookAndFeel();

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
class LostDreamAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit LostDreamAudioProcessorEditor(LostDreamAudioProcessor&);
    ~LostDreamAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void configureRotaryKnob(juce::Slider& slider, bool largeKnob);
    void configureVerticalSlider(juce::Slider& slider);
    void drawSection(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title) const;
    void layoutSquareRotary(juce::Rectangle<int> area, juce::Slider& slider);
    void layoutKnobInCell(juce::Rectangle<int> cell, juce::Label& label, juce::Slider& slider,
                          int fixedKnobSize = 0);
    void layoutKnobColumnEven(juce::Rectangle<int> column,
                              std::initializer_list<std::pair<juce::Label*, juce::Slider*>> knobs);
    void layoutKnobGridEven(juce::Rectangle<int> area, int cols, int rows,
                            std::initializer_list<std::pair<juce::Label*, juce::Slider*>> knobs,
                            bool fillColumnsFirst = false);
    void layoutTopPowerButton(juce::Rectangle<int> topRow, juce::ToggleButton& button);
    void updatePowerButtonText(juce::ToggleButton& button);
    void setupPowerButton(juce::ToggleButton& button);
    void updateGateThresholdLabel();

    LostDreamLookAndFeel pluginLookAndFeel;
    LostDreamAudioProcessor& audioProcessor;

    // Gate
    juce::ToggleButton gateOnButton;
    juce::Slider gateThresholdSlider;
    juce::Slider gateAttackSlider;
    juce::Slider gateReleaseSlider;
    juce::Label gateThresholdLabel;
    juce::Label gateAttackLabel;
    juce::Label gateReleaseLabel;
    juce::Label gateThresholdValueLabel;

    // Reverb
    juce::ToggleButton reverbOnButton;
    juce::Slider roomSizeSlider;
    juce::Slider dampingSlider;
    juce::Slider reverbMixSlider;
    juce::Slider reverbDecaySlider;
    juce::Slider reverbDiffusionSlider;
    juce::Slider reverbPreDelaySlider;
    juce::Slider reverbLowCutSlider;
    juce::Label roomSizeLabel;
    juce::Label dampingLabel;
    juce::Label reverbMixLabel;
    juce::Label reverbDecayLabel;
    juce::Label reverbDiffusionLabel;
    juce::Label reverbPreDelayLabel;
    juce::Label reverbLowCutLabel;

    // Bit crush
    juce::ComboBox crushTypeCombo;
    juce::Slider crushMixSlider;
    juce::Label crushMixLabel;

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
    std::unique_ptr<SliderAttachment> gateAttackAttachment;
    std::unique_ptr<SliderAttachment> gateReleaseAttachment;

    std::unique_ptr<ButtonAttachment> reverbOnAttachment;
    std::unique_ptr<SliderAttachment> roomSizeAttachment;
    std::unique_ptr<SliderAttachment> dampingAttachment;
    std::unique_ptr<SliderAttachment> reverbMixAttachment;
    std::unique_ptr<SliderAttachment> reverbDecayAttachment;
    std::unique_ptr<SliderAttachment> reverbDiffusionAttachment;
    std::unique_ptr<SliderAttachment> reverbPreDelayAttachment;
    std::unique_ptr<SliderAttachment> reverbLowCutAttachment;
    
    std::unique_ptr<ComboBoxAttachment> crushTypeAttachment;
    std::unique_ptr<SliderAttachment> crushMixAttachment;

    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<ComboBoxAttachment> driveModeAttachment;
    std::unique_ptr<SliderAttachment> preGainAttachment;
    std::unique_ptr<SliderAttachment> postGainAttachment;

    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LostDreamAudioProcessorEditor)
};
