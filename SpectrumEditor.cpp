#pragma once

#include "SpectrumEditor.h"
#include "Spectrum.h"

SpectrumEditor::SpectrumEditor(Spectrum& spectrum) : spectrum(spectrum)
{
    // Initialise spectrum points:
    for (auto i = 0; i < spectrum.getNFreqs(); i++)
    {
        SpectrumPoint* point = new SpectrumPoint((int) i, spectrum);
        spectrumPoints.add(point);
    }
}

void SpectrumEditor::paint (juce::Graphics& g)  
{
    g.fillAll(juce::Colour::fromRGB(50,50,50));
    g.setColour(juce::Colours::red);
    
    juce::Path spectrumPath;
    spectrumPath.startNewSubPath(juce::Point<float> (0,(float) getHeight()));
    for (auto i=0; i < spectrumPoints.size(); i++)
    {
        if (spectrumPoints[i] == nullptr) continue;
        // Draw pas
        updateDisplayCoords(spectrumPoints[i]);
        juce::Point<float>& coords = spectrumPoints[i]->displayCoords;
        spectrumPath.lineTo(coords);
        
        // Draw circles as points:
        if (spectrumPoints[i]->selected)
            g.setColour(juce::Colours::rosybrown);
        else
            g.setColour(juce::Colours::red);
        float circleRadius = boundaryRadius;
        g.fillEllipse(coords.x - circleRadius, coords.y - circleRadius, 2*circleRadius, 2*circleRadius);
    }
    spectrumPath.lineTo(juce::Point<float> ((float) getWidth(),(float) getHeight()));
    spectrumPath.closeSubPath();
    g.setColour(juce::Colours::red);
    g.strokePath (spectrumPath, juce::PathStrokeType (1.0f));
}


void SpectrumEditor::mouseDrag (const juce::MouseEvent& event)  
{

    // Determine if a point has been selected
    
    for (auto i = 0; i < spectrumPoints.size(); i++)
    {

        juce::Point<float>& pointPosition = spectrumPoints[i]->displayCoords;
        if (inBoundary(pointPosition, event.position))
        {
            // Mouse is close to peak i
            if (pointSelected) pointSelected->selected = false;
            pointSelected = spectrumPoints[i];
            pointSelected->selected = true;
            
            juce::String debugMsg = "Selected point #" + juce::String((int) i);
            DBG(debugMsg);
            break;
        }
    }

    if (pointSelected != nullptr)  // point selected
    {
        float mag = coordsToMagnitude(event.position);
        pointSelected->updateMagnitude(mag);
        repaint();
    }
}

void SpectrumEditor::mouseUp (const juce::MouseEvent& event)  
{
    const juce::ModifierKeys& modifierKeys = event.mods;
    if (modifierKeys.isLeftButtonDown())
    {
        if (pointSelected != nullptr)
            pointSelected->selected = false;
    }
    else if (modifierKeys.isRightButtonDown()) // Right click clears the spectrum
    {
        clearSpectrum();
        repaint();
    }
}

void SpectrumEditor::refreshPoints()
{
    for (SpectrumPoint* p : spectrumPoints) p->fromSpectrum();
}

void SpectrumEditor::clearSpectrum()
{
    for (SpectrumPoint* p : spectrumPoints) p->updateMagnitude(0.0f);
}

inline float  SpectrumEditor::coordsToMagnitude(const juce::Point<float>& point)
{
    return 1 - (point.y + bottomPadding)/getHeight();
}
inline void  SpectrumEditor::updateDisplayCoords(SpectrumPoint* point)
{
    point->displayCoords.x = ((float) point->index/spectrum.getNFreqs()) * getWidth() + leftPadding;
    point->displayCoords.y = (1 - point->magnitude)*getHeight() - bottomPadding;
}


inline bool  SpectrumEditor::inBoundary(const juce::Point<float>& circleCenter, const juce::Point<float>& testPoint)
{
    return abs(circleCenter.x - testPoint.x) < boundaryRadius;
}



SpectrumEditor::SpectrumPoint::SpectrumPoint(int i, Spectrum& spectrum) : index(i), spectrum(spectrum)
{
    fromSpectrum();
}

inline void SpectrumEditor::SpectrumPoint::updateMagnitude(float mag)
{
    magnitude = mag;
    spectrum.setMagnitude(index, mag);
}
inline void SpectrumEditor::SpectrumPoint::fromSpectrum()
{
    magnitude = spectrum.getMagnitude(index);
}
   
inline float SpectrumEditor::SpectrumPoint::getFrequency()
{
    return spectrum.getFrequency(index);
}