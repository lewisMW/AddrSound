#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class Spectrum
{
public:
    Spectrum(int N, float minFreq, float maxFreq) : size(N), minFreq(minFreq), maxFreq(maxFreq)
    {
        freqRange = maxFreq - minFreq;
        // Initialise spectrum to all 0
        for (auto i = 0; i < N; i++)
        {
            magnitudes.add(0.0f);
            float freq = freqFromIndex(i);
            freqs.add(freq);
        }
    }

    // Logarithmically scale frequencies to ensure less freqs within lower range of human hearing.
    float freqFromIndex(int freqIndex)
    {
        //TODO pow(2.0, (inputFreq - minFreq));
        return minFreq * (float) freqIndex;
    }

    float getFrequency(int index)
    {
        return (freqs[index] - minFreq) / freqRange;
    }
    void setMagnitude(int index, float mag)
    {
        magnitudes.set(index, mag);
    }
    float getMagnitude(int index)
    {
        return magnitudes[index];
    }

    juce::Array<float> magnitudes;
    juce::Array<float> freqs;
    int size;
private:
    const float minFreq;
    const float maxFreq;
    float freqRange;
};