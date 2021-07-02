#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "TimeSlider.h"
#include "Spectrum.h"
#include "SpectrumEditor.h"

TimeSlider::TimeSlider(Spectrum& spectrum, SpectrumEditor& spectrumEditor) : spectrum(spectrum), spectrumEditor(spectrumEditor), time(0.0f) 
{
    spectrum.updateKeyFrameTimes(keyFrameTimes);
    spectrum.timeSlider = this;
}

void TimeSlider::paint (juce::Graphics& g) 
{
    g.fillAll(juce::Colour::fromRGB(30,30,30));

    float height = (float) getHeight();
    float paddedWidth = (float) getWidth();
    float width = paddedWidth - leftPadding - rightPadding;
    float yCenter = height / 2;
    //float xCenter = width / 2;
    float duration = spectrum.getDuration();
    float keyFramePeriod = duration / (spectrum.getNKeyFrames() - 1);

    // Center line
    g.setColour(juce::Colours::indianred);
    juce::Path centerLine;
    centerLine.startNewSubPath(juce::Point<float> (leftPadding, yCenter));
    centerLine.lineTo(juce::Point<float> (paddedWidth - rightPadding, yCenter));
    centerLine.closeSubPath();
    g.strokePath (centerLine, juce::PathStrokeType (centerLineThickness));

    // Keyframe grid:
    g.setColour(juce::Colours::grey);
    for (float t=0.0f; t <= duration+0.5*keyFramePeriod; t += keyFramePeriod)
    {
        float x = timeToX(duration, width, t);
        g.fillRect(juce::Rectangle<float>(x - keyFrameThickness/2 , topPadding, keyFrameThickness, height - bottomPadding));
    }

    // Time cursor (circle)
    g.setColour(juce::Colours::red);    
    g.fillEllipse(timeToX(duration, width, time) - circleRadius, yCenter - circleRadius, 2*circleRadius, 2*circleRadius);

    // Keyframe markers:
    for (float t : keyFrameTimes)
    {
        float x = timeToX(duration, width, t);
        g.fillRect(juce::Rectangle<float>(x - keyFrameThickness/2 , topPadding, keyFrameThickness, height - bottomPadding));
    }
}

void TimeSlider::mouseDrag (const juce::MouseEvent& event) 
{
    float duration = spectrum.getDuration();
    float paddedWidth = (float) getWidth();
    float width = paddedWidth - leftPadding - rightPadding;

    float x = event.position.x;
    // Set bounds:
    if (x < leftPadding || x > leftPadding + width)
        return;

    time = xToTime(duration, width, x);

    spectrum.setTime(time);
    repaint();
    spectrumEditor.refreshPoints();
    spectrumEditor.repaint();
}

void TimeSlider::mouseUp (const juce::MouseEvent& event) 
{
    // Snap time to key frame grid:
    float keyFramePeriod = spectrum.getDuration() / (spectrum.getNKeyFrames() - 1);
    float deltaTime = (float) fmod(time, keyFramePeriod);
    float roundDirection = deltaTime - keyFramePeriod/2;

    if (roundDirection < 0) time -= deltaTime;
    else time += (keyFramePeriod - deltaTime);

    spectrum.setTime(time);
    repaint();
    spectrumEditor.refreshPoints();
    spectrumEditor.repaint();
}

void TimeSlider::refreshKeyFrameMarkers()
{
    spectrum.updateKeyFrameTimes(keyFrameTimes);
    repaint();
}

inline float TimeSlider::timeToX(float duration, float width, float t)
{
    return leftPadding + width * (t / duration);
}

inline float TimeSlider::xToTime(float duration, float width, float x)
{
    return (x - leftPadding) / width * duration;
}