#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class AdditiveSpectrum;
class SpectrumEditor;

class TimeSlider : public juce::Component
{
public:
    TimeSlider(AdditiveSpectrum& spectrum, SpectrumEditor& spectrumEditor);

    void paint (juce::Graphics& g) override;

    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

    void refreshKeyFrameMarkers();

    void playSound();

private:
    const float leftPadding = 5.0f;
    const float rightPadding = 5.0f;
    const float topPadding = 2.0f;
    const float bottomPadding = 2.0f;
    const float centerLineThickness = 2.0f;
    const float circleRadius= 6.0f;
    const float keyFrameThickness = 2.0f;

    const float playSamplePeriod;// in seconds
    juce::Array<float> keyFrameTimes;

    AdditiveSpectrum& spectrum;
    SpectrumEditor& spectrumEditor;
    class PlayerTimer : juce::Timer
    {
    public:
        PlayerTimer(TimeSlider* tS);
        void timerCallback() override;
        void play();
        TimeSlider* timeSlider;
        AdditiveSpectrum* spectrum;
        int timerInterval;
    } playerTimer;


    inline float timeToX(float duration, float width, float t);
    inline float xToTime(float duration, float width, float x);
    inline void repaintAll();
};