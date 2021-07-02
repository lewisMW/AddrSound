#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class Spectrum;
class SpectrumEditor;

class TimeSlider : public juce::Component
{
public:
    TimeSlider(Spectrum& spectrum, SpectrumEditor& spectrumEditor);

    void paint (juce::Graphics& g) override;

    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

    void refreshKeyFrameMarkers();

private:
   
    const float leftPadding = 5.0f;
    const float rightPadding = 5.0f;
    const float topPadding = 2.0f;
    const float bottomPadding = 2.0f;
    const float centerLineThickness = 2.0f;
    const float circleRadius= 6.0f;
    const float keyFrameThickness = 2.0f;
    float time;
    juce::Array<float> keyFrameTimes;
    Spectrum& spectrum;
    SpectrumEditor& spectrumEditor;

    inline float timeToX(float duration, float width, float t);
    inline float xToTime(float duration, float width, float x);
};