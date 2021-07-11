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
#include "EffectSettings.h"

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
    void saveSpectrum();
    void loadSpectrum();
    void loadReferenceFile();
    void loadMidi();
    //==============================================================================
    bool keyPressed(const juce::KeyPress&, juce::Component*) override;
    void setKeyboardNoteBindings();

    


private:
    //==============================================================================   
    // 1. Audio :
    float level;
    juce::OwnedArray<WavetableOscillator> oscillators;
    juce::AudioSampleBuffer sineTable;
    juce::AudioSampleBuffer squareTable;
    const unsigned int tableSize = 128;
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParameters;

    // 2. Spectrum Data :
    AdditiveSpectrum additiveSpectrum;
    FFTSpectrum refSpectrum;

    // 3. GUI Elements:
    SpectrumEditor spectrumEditor;
    TimeSlider timeSlider;
    class ToolsButton : public juce::ComboBox
    {
    public:
        ToolsButton();
        enum ItemIDs {SaveID=1, LoadID, MidiID, GainID, VibratoID, DistortionID, ReverbID, LoadRefID};
    } toolsButton;

    juce::Slider refAudioPositionSlider;

    EffectSettings effectSettings;

    // 4. Audio Reference File Playing
    std::atomic<bool> refPlaying;
    std::shared_ptr<Spectrum::Peaks> fftPeaks;
    std::array<std::shared_ptr<Spectrum::Peaks>,8> circularPointerBuffer;
    int circularPointerBufferIndex;

    // 5. MIDI Playback
    juce::MidiFile mFile;
    class MidiTimer : public juce::HighResolutionTimer
    {
    public:
        MidiTimer(AdditiveSpectrum& additiveSpectrum, TimeSlider& timeSlider, std::atomic<double>* midiControlState);
        void hiResTimerCallback() override;
        void play(const juce::MidiMessageSequence* track);
        void scheduleNextNote();

        const juce::MidiMessageSequence* midiTrack = nullptr;
        const juce::MidiMessage* currentMsg= nullptr;
        int nEvents;
        int eventIndex;
        double currentTime;
        std::atomic<double>* midiControlState;
        AdditiveSpectrum& additiveSpectrum;
        TimeSlider& timeSlider;
    } midiPlayer;

    // 6. Other:
    juce::HashMap<int, int> keyboardNoteBindings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
