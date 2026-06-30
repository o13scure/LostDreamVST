#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>

class FDNReverb
{
public:
    static constexpr int kNumDelays = 16;

    FDNReverb() = default;
    ~FDNReverb() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setDecay(float newDecay);
    void setDiffusion(float d);
    void setPreDelayMs(float ms);
    void setLowCutHz(float hz);
    void setDamping(float d);
    void setMix(float wet);
    void setSize(float s);
    void setStereoOffset(float o);

    void processChannel(float* samples, int numSamples);

private:
    struct DelayLineState
    {
        juce::dsp::DelayLine<float> line;
        float gain = 0.85f;
        float z1 = 0.0f;
    };

    std::array<DelayLineState, (size_t)kNumDelays> delays;
    std::array<int, (size_t)kNumDelays> delayLengths{};
    std::array<float, (size_t)kNumDelays> inputWeights{};
    std::array<float, (size_t)kNumDelays> outputWeights{};

    juce::dsp::DelayLine<float> preDelayLine;

    double sampleRate = 48000.0;
    float preDelaySamples = 0.0f;
    float preDelayMs = 0.0f;
    float decay = 0.85f;
    float damping = 0.2f;
    float diffusion = 0.5f;
    float mix = 0.5f;
    float roomSize = 0.5f;
    float stereoOffset = 0.0f;
    float lowCutHz = 80.0f;
    float lowCutAlpha = 0.0f;

    float lowCutState = 0.0f;
    float lowCutPrevInput = 0.0f;

    void buildDelayLengths();
    void computeWeights();
    void updateDecayGains();
    void updateLowCutCoefficient();
};
