#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "Spectrum.h"

class SpectrumEditor : public juce::Component
{
public:
    SpectrumEditor(Spectrum& spectrum) : spectrum(spectrum)
    {
        // Initialise spectrum points:
        for (auto i = 0; i < spectrum.size; i++)
        {
            SpectrumPoint* point = new SpectrumPoint((int) i, spectrum);
            spectrumPoints.add(point);
        }
    }
    ~SpectrumEditor() override
    {
        spectrumPoints.clear(true);
    }


    void paint (juce::Graphics& g) override
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


    void mouseDrag (const juce::MouseEvent& event) override
    {

        // Determine if a point has been selected
        
        for (auto i = 0; i < spectrumPoints.size(); i++)
        {

            juce::Point<float>& pointPosition = spectrumPoints[i]->displayCoords;
            if (inBoundary(pointPosition, event.position))
            {
                // Mouse is close to peak i
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

    void mouseUp (const juce::MouseEvent& event) override
    {
        if (pointSelected != nullptr)
            pointSelected->selected = false;
        DBG("EVENT: Mouse up");
    }

    
private:
    struct SpectrumPoint
    {
        SpectrumPoint(int i, Spectrum& spectrum) : index(i), spectrum(spectrum)
        {
            fromSpectrum();
        }
        float magnitude;
        float frequency;
        juce::Point<float> displayCoords;
        bool selected = false;
        const int index;
        Spectrum& spectrum;

        inline void updateMagnitude(float mag)
        {
            magnitude = mag;
            spectrum.setMagnitude(index, mag);
        }
        inline void fromSpectrum()
        { 
            frequency = spectrum.getFrequency(index);
            magnitude = spectrum.getMagnitude(index);
        }
    };
    juce::OwnedArray<SpectrumPoint> spectrumPoints;
    inline float coordsToMagnitude(const juce::Point<float>& point)
    {
        return 1 - (point.y + bottomPadding)/getHeight();
    }
    inline void updateDisplayCoords(SpectrumPoint* point)
    {
        point->displayCoords.x = ((float) point->index/spectrum.size) * getWidth() + leftPadding;
        point->displayCoords.y = (1 - point->magnitude)*getHeight() - bottomPadding;
    }

    
    // Point selected:
    SpectrumPoint* pointSelected = nullptr;

    const float bottomPadding = 10.0f;
    const float leftPadding = 7.0f;
    const float boundaryRadius = 5.0f;
    inline bool inBoundary(const juce::Point<float>& circleCenter, const juce::Point<float>& testPoint)
    {
        return abs(circleCenter.x - testPoint.x) < boundaryRadius;
    }
    
    Spectrum& spectrum;
};