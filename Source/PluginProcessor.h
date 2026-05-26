/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

struct Settings {
    float drive{ 6.f }, crush{ 0.f }, output{ 0.f }, mix{ 100.f },
        preGain{ 0.f }, postGain{ 0.f },
        gateThreshold{ -40.f }, gateRatio{ 10.f }, gateAttack{ 5.f }, gateRelease{ 100.f },
        roomSize{ 0.5f }, damping{ 0.5f }, reverbMix{ 0.3f }, width{ 1.0f };
    int driveMode{ 0 };
    bool gateOn{ true }, reverbOn{ true };
};



enum class DriveMode { Soft, Hard, Foldback };

Settings getSettings(juce::AudioProcessorValueTreeState& apvts);


//==============================================================================
/**
*/
class SaturatorAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SaturatorAudioProcessor();
    ~SaturatorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;


    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();
    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "Parameters", createParameterLayout()};

    void processOverdriveBlock(const juce::dsp::ProcessContextReplacing<float>& ctx);

private:
    using Filter = juce::dsp::IIR::Filter<float>;

    juce::dsp::NoiseGate<float> gate;
    juce::dsp::Reverb reverb;
    juce::AudioBuffer<float> dryBuffer;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaturatorAudioProcessor)
};
