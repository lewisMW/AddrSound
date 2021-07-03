#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "TimeSlider.h"
#include "Spectrum.h"
#include "SpectrumEditor.h"

TimeSlider::TimeSlider(Spectrum& spectrum, SpectrumEditor& spectrumEditor)
    : spectrum(spectrum), spectrumEditor(spectrumEditor),
      playSamplePeriod(spectrum.getDuration()/(4*(spectrum.getNKeyFrames())) ),
      playerTimer(this)
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
    float time = spectrum.getTime();

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
    const juce::ModifierKeys& modifierKeys = event.mods;
    if (modifierKeys.isLeftButtonDown())
    {
        if (spectrum.getPlayState() != Spectrum::PlayState::PlayingSound)
        {
            float time = spectrum.getTime();
            float duration = spectrum.getDuration();
            float paddedWidth = (float) getWidth();
            float width = paddedWidth - leftPadding - rightPadding;

            float x = event.position.x;
            // Set bounds:
            if (x < leftPadding || x > leftPadding + width)
                return;

            time = xToTime(duration, width, x);

            spectrum.setTime(time);
            repaintAll();
        }
    }
}

void TimeSlider::mouseUp (const juce::MouseEvent& event) 
{
    if (spectrum.getPlayState() != Spectrum::PlayState::PlayingSound)
    {
        // Get nearest keyframe grid time:
        float time = spectrum.getTime();
        float keyFramePeriod = spectrum.getDuration() / (spectrum.getNKeyFrames() - 1);
        float deltaTime = (float) fmod(time, keyFramePeriod);
        float roundDirection = deltaTime - keyFramePeriod/2;

        if (roundDirection < 0) time -= deltaTime;
        else time += (keyFramePeriod - deltaTime);

        const juce::ModifierKeys& modifierKeys = event.mods;
        if (modifierKeys.isLeftButtonDown())
        {
            // Snap time to key frame grid:
            spectrum.setTime(time);
        }
        else if (modifierKeys.isRightButtonDown()) 
        {
            // Right click deletes this keyframe
            spectrum.deleteKeyframe(time);
        }
        repaintAll();
        
    }
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

inline void TimeSlider::repaintAll()
{
    repaint();
    spectrumEditor.refreshPoints();
    spectrumEditor.repaint();
}

// PlayTimer automatically plays the sound

void TimeSlider::playSound()
{
    playerTimer.play();
}

TimeSlider::PlayerTimer::PlayerTimer(TimeSlider* tS)
{
    spectrum = &tS->spectrum;
    timeSlider = tS;
    timerInterval = juce::roundToInt<float>(tS->playSamplePeriod*1000);
}

void TimeSlider::PlayerTimer::timerCallback()
{
    float time = spectrum->getTime();
    float duration = spectrum->getDuration();

    if(time + timeSlider->playSamplePeriod < duration)
    {
        time += timeSlider->playSamplePeriod;
        spectrum->setTime(time);
    }
    else
    {
        stopTimer();
        spectrum->setPlayState(Spectrum::PlayState::Stopped);
    }
    timeSlider->repaintAll();
}
void TimeSlider::PlayerTimer::play()
{   
    if (spectrum->getPlayState() == Spectrum::PlayState::PlayingSound)
    {
        stopTimer();
    }
    spectrum->setTime(0.0f);
    spectrum->setPlayState(Spectrum::PlayState::PlayingSound);
    startTimer(timerInterval);
    timeSlider->repaintAll();
}