#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

class WavetableOscillator {
public:
    WavetableOscillator(const juce::AudioSampleBuffer& waveTableToUse, float sampleRate) : wavetable(waveTableToUse), tableSize(wavetable.getNumSamples() - 1), fs(sampleRate)
    {
        jassert(wavetable.getNumChannels() == 1);
    }

    void setFrequency(float freq)
    {
        tableDelta = freq * ((float) tableSize / fs);
    }

    void setAmplitude(float a)
    {
        amplitude = a;
    }

    forcedinline float getNextSample() noexcept
    {
        if (amplitude == 0.0f) return 0.0f;
        
        auto index0 = (unsigned int) currentIndex;
        auto index1 = index0 + 1;
        auto frac = currentIndex - (float) index0;

        auto* table = wavetable.getReadPointer(0);
        auto value0 = table[index0];
        auto value1 = table[index1];

        auto currentSample = value0 + frac * (value1 - value0);
        currentIndex += tableDelta;
        while (currentIndex > (float) tableSize)
            currentIndex -= (float) tableSize;

        return amplitude*currentSample;
    }

private:
    const juce::AudioSampleBuffer& wavetable;
    const int tableSize;
    const float fs;
    float amplitude = 1.0f;
    float currentIndex = 0.0f;
    float tableDelta = 0.0f;
};