#pragma once

// CMake builds don't use an AppConfig.h, so it's safe to include juce module headers
// directly. If you need to remain compatible with Projucer-generated builds, and
// have called `juce_generate_juce_header(<thisTarget>)` in your CMakeLists.txt,
// you could `#include <JuceHeader.h>` here instead, to make all your module headers visible.
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "AdditiveSpectrum.h"
#include "SpectrumEditor.h"
#include "FFTSpectrum.h"
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

    void toolsMenuSelect();
    void loadReferenceFile();
    //==============================================================================
    bool keyPressed(const juce::KeyPress&, juce::Component*) override;
    void setKeyboardNoteBindings();

    


private:
    //==============================================================================   
    // 1. Audio :
    float level;
    juce::OwnedArray<WavetableOscillator> oscillators;
    juce::AudioSampleBuffer sineTable;
    const unsigned int tableSize = 128;

    // 2. Data :
    AdditiveSpectrum additiveSpectrum;
    FFTSpectrum refSpectrum;

    // 3. GUI Elements:
    SpectrumEditor spectrumEditor;
    TimeSlider timeSlider;
    class ToolsButton : public juce::ComboBox
    {
    public:
        ToolsButton();
        enum ItemIDs {EQID=1, DistortionID, NoiseID, ReverbID, LoadFileID};
    } toolsButton;

    juce::Slider refAudioPositionSlider;

    // 4. Audio Reference File Playing
    std::atomic<bool> refPlaying;
    std::shared_ptr<FFTSpectrum::FFTPeaks> fftPeaks;
    std::array<std::shared_ptr<FFTSpectrum::FFTPeaks>,8> circularPointerBuffer;
    int circularPointerBufferIndex;
    
    // 5. Other:
    juce::HashMap<int, int> keyboardNoteBindings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
