#include "FDNReverb.h"

namespace
{
    constexpr int kBaseDelays[FDNReverb::kNumDelays]{
        1427, 1601, 1867, 2029, 2251, 2399, 2677, 2953,
        3169, 3449, 3727, 4003, 4253, 4561, 4871, 5101
    };
    constexpr float kReferenceDelaySec = 0.1f;
    constexpr float kBeta = 2.0f / (float)FDNReverb::kNumDelays;
}

// ============================================================
// SETTERS
// ============================================================

void FDNReverb::setDecay(float newDecay)
{
    decay = juce::jlimit(0.0f, 0.999f, newDecay);
    updateDecayGains();
}

void FDNReverb::setDiffusion(float d)
{
    diffusion = juce::jlimit(0.0f, 1.0f, d);
    buildDelayLengths();
    updateDecayGains();
}

void FDNReverb::setPreDelayMs(float ms)
{
    preDelayMs = juce::jmax(0.0f, ms);
    preDelaySamples = preDelayMs * 0.001f * (float)sampleRate;
    preDelayLine.setDelay(preDelaySamples);
}

void FDNReverb::setLowCutHz(float hz)
{
    lowCutHz = juce::jlimit(20.0f, 500.0f, hz);
    updateLowCutCoefficient();
}

void FDNReverb::setDamping(float d) { damping = juce::jlimit(0.0f, 0.95f, d); }
void FDNReverb::setMix(float wet) { mix = juce::jlimit(0.0f, 1.0f, wet); }
void FDNReverb::setSize(float s) { roomSize = juce::jlimit(0.0f, 1.0f, s); buildDelayLengths(); updateDecayGains(); }
void FDNReverb::setStereoOffset(float o) { stereoOffset = o; buildDelayLengths(); updateDecayGains(); }

// ============================================================
// PREPARE / RESET
// ============================================================

void FDNReverb::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    juce::dsp::ProcessSpec monoSpec{ sampleRate, spec.maximumBlockSize, 1 };
    const int maxDelaySamples = (int)std::ceil(0.12 * sampleRate) + 64;

    for (auto& d : delays)
    {
        d.line.reset();
        d.line.prepare(monoSpec);
        d.line.setMaximumDelayInSamples(maxDelaySamples);
        d.z1 = 0.0f;
    }

    preDelayLine.reset();
    preDelayLine.prepare(monoSpec);
    preDelayLine.setMaximumDelayInSamples((int)std::ceil(0.2 * sampleRate) + 1);

    computeWeights();
    buildDelayLengths();
    setPreDelayMs(preDelayMs);
    setLowCutHz(lowCutHz);
    updateDecayGains();
}

void FDNReverb::reset()
{
    for (auto& d : delays)
    {
        d.line.reset();
        d.z1 = 0.0f;
    }
    preDelayLine.reset();
    lowCutState = 0.0f;
    lowCutPrevInput = 0.0f;
}

// ============================================================
// КОЭФФИЦИЕНТЫ
// ============================================================

void FDNReverb::buildDelayLengths()
{
    const float rateScale = (float)(sampleRate / 48000.0);
    const float sizeScale = 0.5f + roomSize;

    for (int i = 0; i < kNumDelays; ++i)
    {
        const float diffScale = 1.0f + stereoOffset
            + diffusion * (((i % 5) - 2) * 0.003f);
        int scaled = (int)std::round((float)kBaseDelays[i]
            * rateScale * sizeScale * diffScale);
        scaled = juce::jlimit(64, (int)std::ceil(0.11 * sampleRate), scaled);
        delayLengths[(size_t)i] = scaled;
        delays[(size_t)i].line.setDelay((float)scaled);
    }
}

void FDNReverb::computeWeights()
{
    const float norm = std::sqrt((float)kNumDelays);
    for (int i = 0; i < kNumDelays; ++i)
    {
        inputWeights[(size_t)i] = ((i % 2) == 0 ? 1.0f : -1.0f) / norm;
        outputWeights[(size_t)i] = 1.0f / norm;
    }
}

void FDNReverb::updateDecayGains()
{
    const float refSamples = kReferenceDelaySec * (float)sampleRate;
    for (int i = 0; i < kNumDelays; ++i)
    {
        delays[(size_t)i].gain = std::pow(decay,
            (float)delayLengths[(size_t)i] / refSamples);
    }
}

void FDNReverb::updateLowCutCoefficient()
{
    const float rc = 1.0f / (juce::MathConstants<float>::twoPi * lowCutHz);
    lowCutAlpha = rc / (rc + 1.0f / (float)sampleRate);
}

// ============================================================
// ОСНОВНОЙ ПРОЦЕСС
// ============================================================

void FDNReverb::processChannel(float* samples, int numSamples)
{
    // Локальные C-массивы на стеке — без std::array::operator[] накладных
    float state[kNumDelays];
    float filtered[kNumDelays];

    for (int n = 0; n < numSamples; ++n)
    {
        // ----- Вход: pre?delay + low?cut --------------------
        const float dry = samples[n];
        preDelayLine.pushSample(0, dry);
        float x = preDelayLine.popSample(0);

        lowCutState = lowCutAlpha * (lowCutState + x - lowCutPrevInput);
        lowCutPrevInput = x;
        x = lowCutState;

        // ----- Проход 1: читаем задержки + выходной сигнал ---
        float wet = 0.0f;
        for (int i = 0; i < kNumDelays; ++i)
        {
            state[i] = delays[i].line.popSample(0);
            wet += outputWeights[i] * state[i];
        }

        // ----- Проход 2: фильтрация + матрица (O(N)) + push ---
        float sumAll = 0.0f;
        for (int j = 0; j < kNumDelays; ++j)
        {
            // Однополюсный absorption?фильтр — инлайн
            const float absorbed = delays[j].z1 * damping
                + state[j] * (1.0f - damping);
            delays[j].z1 = absorbed;
            filtered[j] = absorbed * delays[j].gain;
            sumAll += filtered[j];
        }

        // Хаусхолдер за O(N): A * v = v - ? * (? v) * u
        const float betaSum = kBeta * sumAll;
        for (int i = 0; i < kNumDelays; ++i)
        {
            // feedback = filtered[i] - ? * sumAll
            const float fb = filtered[i] - betaSum;
            delays[i].line.pushSample(0, fb + inputWeights[i] * x);
        }

        // ----- Выход с dry/wet -----------------------------
        samples[n] = dry * (1.0f - mix) + wet * mix;
    }
}
