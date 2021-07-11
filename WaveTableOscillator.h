#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

class WavetableOscillator {
public:
    WavetableOscillator(const juce::AudioSampleBuffer& waveTableToUse, const juce::AudioSampleBuffer& squareWaveTableToUse, float sampleRate)
       : sineWavetable(waveTableToUse), squareWavetable(squareWaveTableToUse), tableSize(sineWavetable.getNumSamples() - 1), fs(sampleRate),
         vibratoDelta(10 /*Hz*/ * ((float) tableSize / fs))
    {
        jassert(sineWavetable.getNumChannels() == 1);
    }

    void setFrequency(float freq)
    {
        tableDelta = freq * ((float) tableSize / fs);
    }

    void setAmplitude(float a)
    {
        amplitude = a;
    }

    void setVibratoFactor(std::atomic<double>* vibrato)
    {
        vibratoFactor = vibrato;
    }

    void setDistortionFactor(std::atomic<double>* distortion)
    {
        distortionFactor = distortion;
    }

    forcedinline float getNextSample() noexcept
    {
        if (amplitude == 0.0f) return 0.0f;
        
        auto index0 = (unsigned int) currentIndex;
        auto index1 = index0 + 1;
        auto frac = currentIndex - (float) index0;

        auto* table = sineWavetable.getReadPointer(0);
        auto value0 = table[index0];
        auto value1 = table[index1];

        if(distortionFactor && *distortionFactor)
        {
            auto* squareTable = squareWavetable.getReadPointer(0);
            value0 = (1-(float)*distortionFactor) * value0 + (float)*distortionFactor * squareTable[index0];
            value1 = (1-(float)*distortionFactor) * value1 + (float)*distortionFactor * squareTable[index1];
        }

        auto delta = tableDelta;

        if (vibratoFactor && *vibratoFactor)
        {
            delta += (float) *vibratoFactor * table[(unsigned int) vibratoIndex];
            vibratoIndex += vibratoDelta;
            while (vibratoIndex > (float) tableSize)
                vibratoIndex -= (float) tableSize;
            while (vibratoIndex < 0.0f)
                vibratoIndex += (float) tableSize;
        }

        auto currentSample = value0 + frac * (value1 - value0);
        currentIndex += delta;
        while (currentIndex > (float) tableSize)
            currentIndex -= (float) tableSize;
        while (currentIndex < 0.0f)
            currentIndex += (float) tableSize;

        return amplitude*currentSample;
    }

private:
    const juce::AudioSampleBuffer& sineWavetable;
    const juce::AudioSampleBuffer& squareWavetable;
    const int tableSize;
    const float fs;
    float amplitude = 1.0f;
    float currentIndex = 0.0f;
    float tableDelta = 0.0f;

    std::atomic<double>* vibratoFactor;
    const float vibratoDelta;
    float vibratoIndex = 0.0f;

    std::atomic<double>* distortionFactor;
};