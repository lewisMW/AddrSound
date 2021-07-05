#pragma once

#include "SpectrumEditor.h"
#include "AdditiveSpectrum.h"
#include "FFTSpectrum.h"

SpectrumEditor::SpectrumEditor(AdditiveSpectrum& spectrum, FFTSpectrum& refSpectrum)
    : spectrum(spectrum), refSpectrum(refSpectrum)
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

    const juce::Point<float> bottomLeft(0,(float) getHeight() - bottomPadding);
    const juce::Point<float> bottomRight((float) getWidth(),(float) getHeight() - bottomPadding);

    // Reference Spectrum in background, so first:
    if(!refSpectrumPoints.isEmpty())
    {
        g.setColour(juce::Colours::darkgrey);
        juce::Path refSpectrumPath;
        refSpectrumPath.startNewSubPath(bottomLeft);
        for (auto i=0; i < refSpectrumPoints.size(); i++)
        {
            if (refSpectrumPoints[i] == nullptr) continue;
            
            updateDisplayCoords(refSpectrumPoints[i]);
            juce::Point<float>& coords = refSpectrumPoints[i]->displayCoords;
            refSpectrumPath.lineTo(coords);
        }
        refSpectrumPath.lineTo(bottomRight);
        refSpectrumPath.closeSubPath();
        g.strokePath (refSpectrumPath, juce::PathStrokeType (0.5f));

        g.drawText(juce::String(refSpectrum.getFrequency(0))+" Hz", 0, getHeight()-50, 30, 20, juce::Justification::centred);
        g.drawText(juce::String(refSpectrum.getFrequency(refSpectrum.getNFreqs()))+" Hz", (int)bottomRight.x-70, getHeight()-50, 80, 20, juce::Justification::centred);
    }

    // Additive Spectrum in foreground:
    g.setColour(juce::Colours::red);
    
    juce::Path spectrumPath;
    spectrumPath.startNewSubPath(bottomLeft);
    for (auto i=0; i < spectrumPoints.size(); i++)
    {
        if (spectrumPoints[i] == nullptr) continue;
        // Draw path
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
    spectrumPath.lineTo(bottomRight);
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
void SpectrumEditor::refreshRefPoints()
{
    for (SpectrumPoint* p : refSpectrumPoints) p->fromSpectrum();
}

void SpectrumEditor::clearSpectrum()
{
    for (SpectrumPoint* p : spectrumPoints) p->updateMagnitude(0.0f);
}
void SpectrumEditor::clearRefSpectrum()
{
    for (SpectrumPoint* p : refSpectrumPoints) p->updateMagnitude(0.0f);
}

void SpectrumEditor::addRefSpectrum()
{
    refSpectrumPoints.clear(true);
    for (auto i = 0; i < refSpectrum.getNFreqs(); i++)
    {
        SpectrumPoint* point = new SpectrumPoint((int) i, refSpectrum);
        refSpectrumPoints.add(point);
    }
}





inline float  SpectrumEditor::coordsToMagnitude(const juce::Point<float>& point)
{
    return 1.0f - (point.y-topPadding)/(getHeight()-bottomPadding-topPadding);
}
inline void  SpectrumEditor::updateDisplayCoords(SpectrumPoint* point)
{
    point->displayCoords.x = ((float) point->index/point->spectrum.getNFreqs()) * getWidth() + leftPadding;
    point->displayCoords.y = topPadding + (1.0f - point->magnitude)*(getHeight()-bottomPadding-topPadding);
}

inline bool  SpectrumEditor::inBoundary(const juce::Point<float>& circleCenter, const juce::Point<float>& testPoint)
{
    return abs(circleCenter.x - testPoint.x) < boundaryRadius;
}











SpectrumEditor::SpectrumPoint::SpectrumPoint(int i, Spectrum& spectrum)
    : index(i), spectrum(spectrum)
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