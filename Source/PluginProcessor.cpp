/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SaturatorAudioProcessor::SaturatorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SaturatorAudioProcessor::~SaturatorAudioProcessor()
{
}

//==============================================================================
const juce::String SaturatorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SaturatorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SaturatorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SaturatorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SaturatorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SaturatorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SaturatorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SaturatorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SaturatorAudioProcessor::getProgramName (int index)
{
    return {};
}

void SaturatorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SaturatorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const auto numChannels = (juce::uint32) juce::jmax(getTotalNumInputChannels(), getTotalNumOutputChannels());
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, numChannels };

    gate.prepare(spec);
    reverb.prepare(spec);
    reverb.reset();

    dryBuffer.setSize((int) numChannels, samplesPerBlock);
}


void SaturatorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SaturatorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

static float foldback(float x)
{
    if (x > 1.0f || x < -1.0f)
        return (std::abs(x) - 2.0f * std::floor((std::abs(x) + 1.0f) * 0.5f)) * (x >= 0 ? 1.0f : -1.0f);
    return x;
}

void SaturatorAudioProcessor::processOverdriveBlock(const juce::dsp::ProcessContextReplacing<float>& ctx)
{
    auto& block = ctx.getOutputBlock();
    auto settings = getSettings(apvts);

    for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
    {
        auto* data = block.getChannelPointer(ch);
        for (size_t i = 0; i < block.getNumSamples(); ++i)
        {
            float y = data[i] * juce::Decibels::decibelsToGain(settings.preGain);

            const auto driveGain = juce::Decibels::decibelsToGain(settings.drive);

            switch (settings.driveMode)
            {
            case 0: y = std::tanh(y * driveGain); break;
            case 1: y = juce::jlimit(-1.0f, 1.0f, y * driveGain); break;
            case 2: y = foldback(y * driveGain); break;
            default: break;
            }

            y *= juce::Decibels::decibelsToGain(settings.postGain);
            data[i] = y;
        }
    }
}




void SaturatorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (totalNumInputChannels == 0)
        return;

    auto settings = getSettings(apvts);

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (dryBuffer.getNumChannels() != numChannels || dryBuffer.getNumSamples() < numSamples)
        dryBuffer.setSize(numChannels, numSamples, false, false, true);

    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);

    

    if (settings.reverbOn)
    {
        juce::dsp::Reverb::Parameters reverbParams;
        reverbParams.roomSize = settings.roomSize;
        reverbParams.damping = settings.damping;
        reverbParams.wetLevel = settings.reverbMix;
        reverbParams.dryLevel = 1.0f - settings.reverbMix;
        reverbParams.width = settings.width;
        reverbParams.freezeMode = 0.0f;
        reverb.setParameters(reverbParams);
        reverb.process(ctx);
    }

    processOverdriveBlock(ctx);

    if (settings.gateOn)
    {
        gate.setThreshold(settings.gateThreshold);
        gate.setRatio(settings.gateRatio);
        gate.setAttack(settings.gateAttack);
        gate.setRelease(settings.gateRelease);
        gate.process(ctx);
    }

    const float wetMix = settings.mix;
    const float dryMix = 1.0f - wetMix;

    if (wetMix < 1.0f)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            const auto* dry = dryBuffer.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
                wet[i] = dry[i] * dryMix + wet[i] * wetMix;
        }
    }

    buffer.applyGain(juce::Decibels::decibelsToGain(settings.output));
}



//==============================================================================
bool SaturatorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SaturatorAudioProcessor::createEditor()
{
    return new SaturatorAudioProcessorEditor(*this);
}

//==============================================================================
void SaturatorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SaturatorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

Settings getSettings(juce::AudioProcessorValueTreeState& apvts) {
    Settings s;

    // Drive / overdrive
    s.driveMode = (int)apvts.getRawParameterValue("DriveMode")->load();
    s.drive = apvts.getRawParameterValue("Drive")->load();
    s.preGain = apvts.getRawParameterValue("PreGain")->load();
    s.postGain = apvts.getRawParameterValue("PostGain")->load();

    // Gate
    s.gateOn = apvts.getRawParameterValue("GateOn")->load() > 0.5f;
    s.gateThreshold = apvts.getRawParameterValue("GateThreshold")->load();
    s.gateRatio = apvts.getRawParameterValue("GateRatio")->load();
    s.gateAttack = apvts.getRawParameterValue("GateAttack")->load();
    s.gateRelease = apvts.getRawParameterValue("GateRelease")->load();

    // Reverb
    s.reverbOn = apvts.getRawParameterValue("ReverbOn")->load() > 0.5f;
    s.roomSize = apvts.getRawParameterValue("RoomSize")->load();
    s.damping = apvts.getRawParameterValue("Damping")->load();
    s.reverbMix = apvts.getRawParameterValue("ReverbMix")->load();
    s.width = apvts.getRawParameterValue("Width")->load();

    // Output
    s.output = apvts.getRawParameterValue("Output")->load();
    s.mix = apvts.getRawParameterValue("Mix")->load() * 0.01f;

    return s;
}



juce::AudioProcessorValueTreeState::ParameterLayout SaturatorAudioProcessor::createParameterLayout() 
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // GATE
    
    layout.add(std::make_unique<juce::AudioParameterBool>(
    "GateOn", "Gate On", true));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "GateThreshold", "Gate Threshold",
        juce::NormalisableRange<float>(-80.0f, 0.0f, 0.1f),
        -40.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "GateRatio", "Gate Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f),
        10.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "GateAttack", "Gate Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f),
        5.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "GateRelease", "Gate Release",
        juce::NormalisableRange<float>(1.0f, 1000.0f, 1.0f),
        100.0f));

    
    // DRIVE

    layout.add(std::make_unique<juce::AudioParameterChoice>(
    "DriveMode", "Drive Mode",
    juce::StringArray { "Soft", "Hard", "Foldback" }, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Drive", "Drive (dB)",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.01f),
        6.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "PreGain", "Pre Gain",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.01f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "PostGain", "Post Gain",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.01f),
        0.0f));

    // REVERB
    
    layout.add(std::make_unique<juce::AudioParameterBool>(
    "ReverbOn", "Reverb On", true));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "RoomSize", "Room Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Damping", "Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ReverbMix", "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.3f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Width", "Width",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Output", "Output",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.01f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Mix", "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SaturatorAudioProcessor();
}
