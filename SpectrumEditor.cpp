#pragma once

#include "SpectrumEditor.h"
#include "AdditiveSpectrum.h"
#include "FFTSpectrum.h"

SpectrumEditor::SpectrumEditor(AdditiveSpectrum& spectrum, FFTSpectrum& refSpectrum)
    : spectrum(spectrum),
      refSpectrum(refSpectrum), refDisplayOffset(0.0f), refDisplayScale(1.0f)
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
    g.setFont(9.0f);

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
            juce::Point<float> coords = refSpectrumPoints[i]->displayCoords;
            coords.x *= refDisplayScale; // Scale this coordinate
            coords.x += refDisplayOffset; // Offset this coordinate
            refSpectrumPath.lineTo(coords);
            if(refSpectrumPoints[i]->marked) // Mark frequency points
            {
                const float circleRadius = boundaryRadius;
                g.fillEllipse(coords.x - circleRadius/2, coords.y - circleRadius/2, circleRadius, circleRadius);
                g.drawText(juce::String(juce::roundToInt<float>(refSpectrumPoints[i]->getFrequency())), (int)coords.x - 10, (int)coords.y - 20, 20, 20, juce::Justification::centred);
            }
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
        juce::Point<float> bottom(coords);
        bottom.y = getHeight() - bottomPadding;
        spectrumPath.lineTo(bottom);
        spectrumPath.lineTo(coords);
        spectrumPath.lineTo(bottom);
        
        // Draw circles as points:
        if (spectrumPoints[i]->selected)
        {
            g.setColour(juce::Colours::rosybrown);
            // Mark frequency:
            g.drawText(juce::String(juce::roundToInt<float>(spectrumPoints[i]->getFrequency())), (int)coords.x - 20, (int)coords.y - 20, 40, 20, juce::Justification::centred);
        }
        else
            g.setColour(juce::Colours::red);
        const float circleRadius = boundaryRadius;
        g.fillEllipse(coords.x - circleRadius, coords.y - circleRadius, 2*circleRadius, 2*circleRadius);
    }
    spectrumPath.lineTo(bottomRight);
    spectrumPath.closeSubPath();
    g.setColour(juce::Colours::red);
    g.strokePath (spectrumPath, juce::PathStrokeType (1.0f));
}






void SpectrumEditor::mouseDrag (const juce::MouseEvent& event)  
{

    const juce::ModifierKeys& modifierKeys = event.mods;
    if (modifierKeys.isLeftButtonDown()) // Change magnitude of additive spectra points
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
                pointSelected->marked = true;
                
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
    else if (modifierKeys.isRightButtonDown()) // Right drag offsets reference spectrum
    {
        float deltaX = (float) event.getDistanceFromDragStartX();
        offsetRefSpectrum(deltaX);
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

void SpectrumEditor::mouseWheelMove(const juce::MouseEvent& /*event*/, const juce::MouseWheelDetails& wheel)
{
    int dir = (wheel.isReversed ? -1 : 1);
    const float offsetScrollGain = 10.0f;
    
    scaleRefSpectrum(wheel.deltaY*dir);
    offsetRefSpectrum(wheel.deltaX*dir*offsetScrollGain);
    repaint();
}






void SpectrumEditor::refreshPoints(bool ref, Spectrum::Peaks* peaks)
{
    juce::OwnedArray<SpectrumPoint>& points = ref ? refSpectrumPoints : spectrumPoints;

    // For marking peaks:
    if (peaks != nullptr && peaks->indexs.size() > 0)
    {
        int nPeaks = (int)peaks->indexs.size();
        int peakIndex = 0;
        int currentPeakIndex = peaks->indexs[peakIndex++];

        for (int i=0; i<points.size(); i++)
        {
            auto p = points[i];
            bool isPeak = false;
            if (currentPeakIndex == i)
            {
                isPeak = true;
                if (nPeaks > peakIndex) currentPeakIndex = peaks->indexs[peakIndex++];
            }
            p->fromSpectrum(isPeak);
        }
    }
    else // no peaks
    {
        for (int i=0; i<points.size(); i++)
        {
            auto p = points[i];
            p->fromSpectrum();
        }
    }
}

void SpectrumEditor::clearSpectrum(bool ref)
{
    juce::OwnedArray<SpectrumPoint>& points = ref ? refSpectrumPoints : spectrumPoints;

    for (SpectrumPoint* p : points) p->updateMagnitude(0.0f);
}

void SpectrumEditor::addRefSpectrum()
{
    refSpectrumPoints.clear(true);
    refDisplayOffset = 0.0f; // Reset offset and scale:
    refDisplayScale = 1.0f;
    for (auto i = 0; i < refSpectrum.getNFreqs(); i++)
    {
        SpectrumPoint* point = new SpectrumPoint((int) i, refSpectrum);
        refSpectrumPoints.add(point);
    }
}
inline void SpectrumEditor::offsetRefSpectrum(float delta)
{
    refDisplayOffset += delta;
    // Thresholds:
    if (refDisplayOffset < -getWidth()/2) refDisplayOffset = (float) -getWidth()/2;
    else if (refDisplayOffset > getWidth()/2) refDisplayOffset = (float) getWidth()/2;
}
inline void SpectrumEditor::scaleRefSpectrum(float delta)
{
    refDisplayScale += delta;
    // Thresholds:
    if (refDisplayScale < 0.25f) refDisplayScale = 0.25f;
    else if (refDisplayScale > 5.0f) refDisplayScale = 5.0f;
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
    marked = false;
    selected = false;
    fromSpectrum();
}

inline void SpectrumEditor::SpectrumPoint::updateMagnitude(float mag)
{
    magnitude = mag;
    spectrum.setMagnitude(index, mag);
}
inline void SpectrumEditor::SpectrumPoint::fromSpectrum(bool isPeak)
{
    magnitude = spectrum.getMagnitude(index);
    marked = isPeak;
}
   
inline float SpectrumEditor::SpectrumPoint::getFrequency()
{
    return spectrum.getFrequency(index);
}