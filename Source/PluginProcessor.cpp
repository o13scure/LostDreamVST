/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
LostDreamAudioProcessor::LostDreamAudioProcessor()
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

LostDreamAudioProcessor::~LostDreamAudioProcessor()
{
}

//==============================================================================
const juce::String LostDreamAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LostDreamAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool LostDreamAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool LostDreamAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double LostDreamAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LostDreamAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int LostDreamAudioProcessor::getCurrentProgram()
{
    return 0;
}

void LostDreamAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String LostDreamAudioProcessor::getProgramName (int index)
{
    return {};
}

void LostDreamAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void LostDreamAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const auto numChannels = juce::jmax(getTotalNumInputChannels(), getTotalNumOutputChannels());
    juce::dsp::ProcessSpec monoSpec { sampleRate, (juce::uint32) samplesPerBlock, 1 };

    customGate.prepare(sampleRate, numChannels);
    bitCrusher.prepare(sampleRate, numChannels);

    for (auto& rev : schroederReverb)
        rev.prepare(monoSpec);
    for (auto& rev : fdnReverb)
        rev.prepare(monoSpec);

    schroederReverb[1].setStereoOffset(0.008f);
    fdnReverb[1].setStereoOffset(0.01f);

    dryBuffer.setSize(numChannels, samplesPerBlock);
}


void LostDreamAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LostDreamAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void LostDreamAudioProcessor::processOverdriveBlock(const juce::dsp::ProcessContextReplacing<float>& ctx)
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




void LostDreamAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
        processReverbBlock(buffer, settings);

    processOverdriveBlock(ctx);

    if (settings.gateOn)
        processGateBlock(buffer, settings);

    processBitCrushBlock(buffer, settings);

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
bool LostDreamAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* LostDreamAudioProcessor::createEditor()
{
    return new LostDreamAudioProcessorEditor(*this);
}

//==============================================================================
void LostDreamAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void LostDreamAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    s.gateAttack = apvts.getRawParameterValue("GateAttack")->load();
    s.gateRelease = apvts.getRawParameterValue("GateRelease")->load();

    // Reverb
    s.reverbOn = apvts.getRawParameterValue("ReverbOn")->load() > 0.5f;
    s.roomSize = apvts.getRawParameterValue("RoomSize")->load();
    s.damping = apvts.getRawParameterValue("Damping")->load();
    s.reverbMix = apvts.getRawParameterValue("ReverbMix")->load();
    s.reverbDecay = apvts.getRawParameterValue("ReverbDecay")->load();
    s.reverbDiffusion = apvts.getRawParameterValue("ReverbDiffusion")->load();
    s.reverbPreDelay = apvts.getRawParameterValue("ReverbPreDelay")->load();
    s.reverbLowCut = apvts.getRawParameterValue("ReverbLowCut")->load();

    s.crushType = (int) apvts.getRawParameterValue("CrushType")->load();
    s.crushMix = apvts.getRawParameterValue("CrushMix")->load();
    s.crushBits = apvts.getRawParameterValue("CrushBits")->load();

    // Output
    s.output = apvts.getRawParameterValue("Output")->load();
    s.mix = apvts.getRawParameterValue("Mix")->load() * 0.01f;

    return s;
}



juce::AudioProcessorValueTreeState::ParameterLayout LostDreamAudioProcessor::createParameterLayout() 
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
        "GateAttack", "Gate Attack",
        juce::NormalisableRange<float>(0.01f, 50.0f, 0.01f, 0.45f),
        5.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "GateRelease", "Gate Release",
        juce::NormalisableRange<float>(1.0f, 1000.0f, 0.1f, 0.35f),
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

    // BIT CRUSH

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "CrushType", "Crush Type",
        juce::StringArray { "Full", "Quiet", "Loud" }, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "CrushMix", "Crush Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "CrushBits", "Crush Bits",
        juce::NormalisableRange<float>(4.0f, 16.0f, 0.1f),
        8.0f));

    // REVERB
    
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "ReverbOn", "Reverb On", true));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "ReverbType", "Reverb Type",
        juce::StringArray { "Schroeder", "FDN" }, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "RoomSize", "Room Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Damping", "Damping",
        juce::NormalisableRange<float>(0.05f, 0.95f, 0.001f),
        0.2f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ReverbMix", "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.3f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ReverbDecay", "Reverb Decay",
        juce::NormalisableRange<float>(0.0f, 0.999f, 0.001f),
        0.85f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ReverbDiffusion", "Reverb Diffusion",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ReverbPreDelay", "Reverb Pre Delay",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ReverbLowCut", "Reverb Low Cut",
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.4f),
        80.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ReverbAllpass", "Reverb Allpass",
        juce::NormalisableRange<float>(0.1f, 0.9f, 0.001f),
        0.7f));

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
void CustomNoiseGate::prepare(double newSampleRate, int channels)
{
    sampleRate = newSampleRate;
    numChannels = juce::jmax(1, channels);

    constexpr double envelopeTimeSec = 0.005; // 5 ms RMS detector
    rmsAlpha = (float) std::exp(-1.0 / (envelopeTimeSec * sampleRate));

    updateCoefficients();
    reset();
}

void CustomNoiseGate::reset()
{
    rmsEnvelope = 0.0f;
    gain = 1.0f;
}

void CustomNoiseGate::setThresholdDb(float newThresholdDb)
{
    thresholdDb = newThresholdDb;
}

void CustomNoiseGate::setAttackMs(float newAttackMs)
{
    attackMs = juce::jmax(0.01f, newAttackMs);
    updateCoefficients();
}

void CustomNoiseGate::setReleaseMs(float newReleaseMs)
{
    releaseMs = juce::jmax(1.0f, newReleaseMs);
    updateCoefficients();
}

void CustomNoiseGate::updateCoefficients()
{
    const auto attackSec = attackMs * 0.001f;
    attackStep = juce::jlimit(0.0f, 1.0f, 1.0f / (float) (attackSec * sampleRate));

    const auto releaseSec = releaseMs * 0.001f;
    releaseBeta = (float) std::exp(-1.0 / (releaseSec * sampleRate));
}

void CustomNoiseGate::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int channelsToUse = juce::jmin(numChannels, buffer.getNumChannels());
    const float thresholdLinear = juce::Decibels::decibelsToGain(thresholdDb);

    for (int i = 0; i < numSamples; ++i)
    {
        float monoSquare = 0.0f;
        for (int ch = 0; ch < channelsToUse; ++ch)
        {
            const float x = buffer.getSample(ch, i);
            monoSquare += x * x;
        }
        monoSquare /= (float) channelsToUse;

        // RMS envelope: e[n] = sqrt(alpha * e^2[n-1] + (1-alpha) * x^2[n])
        rmsEnvelope = std::sqrt(rmsAlpha * rmsEnvelope * rmsEnvelope
                                + (1.0f - rmsAlpha) * monoSquare);

        const bool gateOpen = rmsEnvelope >= thresholdLinear;

        if (gateOpen)
        {
            // Attack: g[n] = g[n-1] + (1 / (tau_a * fs)) * (1 - g[n-1])
            gain += attackStep * (1.0f - gain);
        }
        else
        {
            // Release: g[n] = g[n-1] * beta_release
            gain *= releaseBeta;
        }

        for (int ch = 0; ch < channelsToUse; ++ch)
            buffer.setSample(ch, i, buffer.getSample(ch, i) * gain);
    }
}

void LostDreamAudioProcessor::processGateBlock(juce::AudioBuffer<float>& buffer, const Settings& settings)
{
    customGate.setThresholdDb(settings.gateThreshold);
    customGate.setAttackMs(settings.gateAttack);
    customGate.setReleaseMs(settings.gateRelease);
    customGate.process(buffer);
}

void LostDreamAudioProcessor::applyReverbParams(const Settings& settings)
{

    for (auto& rev : fdnReverb)
    {
        rev.setSize(settings.roomSize);
        rev.setDecay(settings.reverbDecay);
        rev.setDiffusion(settings.reverbDiffusion);
        rev.setDamping(settings.damping);
        rev.setPreDelayMs(settings.reverbPreDelay);
        rev.setLowCutHz(settings.reverbLowCut);
        rev.setMix(settings.reverbMix);
    }
}

void BitCrusher::prepare(double newSampleRate, int channels)
{
    sampleRate = newSampleRate;
    numChannels = juce::jmax(1, channels);

    constexpr double envelopeTimeSec = 0.01;
    rmsAlpha = (float) std::exp(-1.0 / (envelopeTimeSec * sampleRate));
    reset();
}

void BitCrusher::reset()
{
    envelope = 0.0f;
}

void BitCrusher::setType(int type) { crushType = juce::jlimit(0, 2, type); }
void BitCrusher::setMix(float wet) { mix = juce::jlimit(0.0f, 1.0f, wet); }
void BitCrusher::setBits(float bits) { targetBits = juce::jlimit(minBits, maxBits, bits); }

float BitCrusher::quantize(float sample, float bits)
{
    const float levels = std::exp2(bits) - 1.0f;
    const float clamped = juce::jlimit(-1.0f, 1.0f, sample);
    return std::round(clamped * levels) / levels;
}

float BitCrusher::effectiveBits(float env) const
{
    env = juce::jlimit(0.0f, 1.0f, env);

    switch (crushType)
    {
    case 0: // Full — constant resolution
        return targetBits;

    case 1: // Quiet — more crush when envelope is low
        return minBits + env * (targetBits - minBits);

    case 2: // Loud — more crush when envelope is high
        return targetBits - env * (targetBits - minBits);

    default:
        return targetBits;
    }
}

void BitCrusher::process(juce::AudioBuffer<float>& buffer)
{
    if (mix <= 0.0001f)
        return;

    const int numSamples = buffer.getNumSamples();
    const int channelsToUse = juce::jmin(numChannels, buffer.getNumChannels());

    for (int i = 0; i < numSamples; ++i)
    {
        float monoSquare = 0.0f;
        for (int ch = 0; ch < channelsToUse; ++ch)
        {
            const float x = buffer.getSample(ch, i);
            monoSquare += x * x;
        }
        monoSquare /= (float) channelsToUse;

        envelope = std::sqrt(rmsAlpha * envelope * envelope + (1.0f - rmsAlpha) * monoSquare);
        const float bits = effectiveBits(envelope);

        for (int ch = 0; ch < channelsToUse; ++ch)
        {
            const float dry = buffer.getSample(ch, i);
            const float crushed = quantize(dry, bits);
            buffer.setSample(ch, i, dry * (1.0f - mix) + crushed * mix);
        }
    }
}

void LostDreamAudioProcessor::processBitCrushBlock(juce::AudioBuffer<float>& buffer, const Settings& settings)
{
    bitCrusher.setType(settings.crushType);
    bitCrusher.setMix(settings.crushMix);
    bitCrusher.setBits(settings.crushBits);
    bitCrusher.process(buffer);
}

void LostDreamAudioProcessor::processReverbBlock(juce::AudioBuffer<float>& buffer, const Settings& settings)
{
    applyReverbParams(settings);

    const int numSamples = buffer.getNumSamples();
    const int channelsToProcess = juce::jmin(buffer.getNumChannels(), 2);

    
    for (int ch = 0; ch < channelsToProcess; ++ch){
        fdnReverb[(size_t) ch].processChannel(buffer.getWritePointer(ch), numSamples);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LostDreamAudioProcessor();
}
