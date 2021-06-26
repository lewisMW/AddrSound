#pragma once

// CMake builds don't use an AppConfig.h, so it's safe to include juce module headers
// directly. If you need to remain compatible with Projucer-generated builds, and
// have called `juce_generate_juce_header(<thisTarget>)` in your CMakeLists.txt,
// you could `#include <JuceHeader.h>` here instead, to make all your module headers visible.
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>

class SineOscillator {
public:
    SineOscillator() {}

    void setFrequency(float freq, float fs) {
        angleDelta = freq * juce::MathConstants<float>::twoPi / fs;
    }

    forcedinline void updateAngle() noexcept
    {
        currentAngle += angleDelta;
        if(currentAngle >= juce::MathConstants<float>::twoPi)
            currentAngle -= juce::MathConstants<float>::twoPi;
    }

    forcedinline float getNextSample() noexcept
    {
        auto currentSample = std::sin(currentAngle);
        updateAngle();
        return currentSample;
    }

private:
    float currentAngle = 0.0f;
    float angleDelta = 0.0f;

};

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

    forcedinline float getNextSample() noexcept
    {
        auto index0 = (unsigned int) currentIndex;
        auto index1 = index0 + 1;
        auto frac = currentIndex - (float) index0;

        auto* table = wavetable.getReadPointer(0);
        auto value0 = table[index0];
        auto value1 = table[index1];

        auto currentSample = value0 + frac * (value1 - value0);
        if ((currentIndex += tableDelta) > (float) tableSize)
            currentIndex -= (float) tableSize;

        return currentSample;
    }

private:
    const juce::AudioSampleBuffer& wavetable;
    const int tableSize;
    const float fs;
    float currentIndex = 0.0f;
    float tableDelta = 0.0f;
};

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public juce::AudioAppComponent, public juce::KeyListener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;
    //==============================================================================
    void prepareToPlay(int,double) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;
    void releaseResources() override;

    void createWaveTable();

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    //==============================================================================
    bool keyPressed(const juce::KeyPress&, juce::Component*) override;

private:
    //==============================================================================
    // Your private member variables go here...
    juce::Random random;
    
    float level = 0.0f;
    juce::OwnedArray<WavetableOscillator> oscillators;
    juce::AudioSampleBuffer sineTable;
    const unsigned int tableSize = 128;

    // GUI Elements:
    juce::Slider freqSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
