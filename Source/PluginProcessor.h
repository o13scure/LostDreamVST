/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>

#include "FDNReverb.h"
#include "SchroederMoorerReverb.h"

struct Settings {
    float drive{ 6.f }, output{ 0.f }, mix{ 100.f },
        preGain{ 0.f }, postGain{ 0.f },
        gateThreshold{ -40.f }, gateAttack{ 5.f }, gateRelease{ 100.f },
        roomSize{ 0.5f }, damping{ 0.2f }, reverbMix{ 0.3f },
        reverbDecay{ 0.85f }, reverbDiffusion{ 0.5f }, reverbPreDelay{ 0.f },
        reverbLowCut{ 80.f }, 
        crushMix{ 0.f }, crushBits{ 8.f };
    int driveMode{ 0 }, crushType{ 0 };
    bool gateOn{ true }, reverbOn{ true };
};



enum class DriveMode { Soft, Hard, Foldback };

//==============================================================================
class CustomNoiseGate
{
public:
    void prepare(double sampleRate, int numChannels);
    void reset();
    void setThresholdDb(float thresholdDb);
    void setAttackMs(float attackMs);
    void setReleaseMs(float releaseMs);
    void process(juce::AudioBuffer<float>& buffer);

private:
    void updateCoefficients();

    double sampleRate = 44100.0;
    int numChannels = 2;

    float thresholdDb = -40.0f;
    float attackMs = 5.0f;
    float releaseMs = 100.0f;

    // RMS envelope: alpha = exp(-1 / (tau * fs)), tau = 5 ms
    float rmsAlpha = 0.99f;
    float attackStep = 0.01f;
    float releaseBeta = 0.999f;

    float rmsEnvelope = 0.0f;
    float gain = 1.0f;
};

//==============================================================================
class BitCrusher
{
public:
    void prepare(double sampleRate, int numChannels);
    void reset();

    void setType(int type);
    void setMix(float wet);
    void setBits(float bits);

    void process(juce::AudioBuffer<float>& buffer);

private:
    static float quantize(float sample, float bits);
    float effectiveBits(float envelope) const;

    double sampleRate = 44100.0;
    int numChannels = 2;

    int crushType = 0;
    float mix = 0.0f;
    float targetBits = 8.0f;
    float minBits = 4.0f;
    float maxBits = 16.0f;

    float rmsAlpha = 0.99f;
    float envelope = 0.0f;
};

Settings getSettings(juce::AudioProcessorValueTreeState& apvts);


//==============================================================================
/**
*/
class LostDreamAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    LostDreamAudioProcessor();
    ~LostDreamAudioProcessor() override;

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
    void processGateBlock(juce::AudioBuffer<float>& buffer, const Settings& settings);
    void processReverbBlock(juce::AudioBuffer<float>& buffer, const Settings& settings);
    void applyReverbParams(const Settings& settings);
    void processBitCrushBlock(juce::AudioBuffer<float>& buffer, const Settings& settings);

private:
    CustomNoiseGate customGate;
    BitCrusher bitCrusher;
    std::array<SchroederMoorerReverb, 2> schroederReverb {};
    std::array<FDNReverb, 2> fdnReverb {};
    juce::AudioBuffer<float> dryBuffer;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LostDreamAudioProcessor)
};
