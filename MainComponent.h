#pragma once

// CMake builds don't use an AppConfig.h, so it's safe to include juce module headers
// directly. If you need to remain compatible with Projucer-generated builds, and
// have called `juce_generate_juce_header(<thisTarget>)` in your CMakeLists.txt,
// you could `#include <JuceHeader.h>` here instead, to make all your module headers visible.
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "Spectrum.h"
#include "SpectrumEditor.h"
#include "TimeSlider.h"
#include "WaveTableOscillator.h"

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
    void setKeyboardNoteBindings();

private:
    //==============================================================================   
    float level = 0.0f;
    juce::OwnedArray<WavetableOscillator> oscillators;
    juce::AudioSampleBuffer sineTable;
    const unsigned int tableSize = 128;

    Spectrum spectrum;

    // GUI Elements:
    SpectrumEditor spectrumEditor;
    TimeSlider timeSlider;

    juce::HashMap<int, int> keyboardNoteBindings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
