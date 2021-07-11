#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "Spectrum.h"

class AdditiveSpectrum;
class FFTSpectrum;

class SpectrumEditor : public juce::Component
{
public:
    SpectrumEditor(AdditiveSpectrum& spectrum, FFTSpectrum& refSpectrum);

    void paint (juce::Graphics& g) override;

    void mouseUp (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    void refreshPoints(bool ref = false, Spectrum::Peaks* peaks = nullptr);
    void multiplyAllPoints(double delta);
    void initPoints();

    void clearSpectrum(bool ref = false);

    void addRefSpectrum();
    
private:
    struct SpectrumPoint
    {
        SpectrumPoint(int i, Spectrum& spectrum);
        float magnitude;
        juce::Point<float> displayCoords;
        bool selected;
        const int index;
        bool marked;
        Spectrum& spectrum;

        inline void updateMagnitude(float mag);
        inline void fromSpectrum(bool isPeak = false);
        inline float getFrequency();
    };
    
    juce::OwnedArray<SpectrumPoint> spectrumPoints;
    juce::OwnedArray<SpectrumPoint> refSpectrumPoints;
    inline float coordsToMagnitude(const juce::Point<float>& point);
    inline void updateDisplayCoords(SpectrumPoint* point);
    inline void scaleRefSpectrum(float delta);
    inline void offsetRefSpectrum(float delta);
    
    // Point selected:
    SpectrumPoint* pointSelected = nullptr;

    const float topPadding = 20.0f;
    const float bottomPadding = 10.0f;
    const float leftPadding = 7.0f;
    const float boundaryRadius = 2.5f;
    inline bool inBoundary(const juce::Point<float>& circleCenter, const juce::Point<float>& testPoint);
    
    AdditiveSpectrum& spectrum;
    FFTSpectrum& refSpectrum;
    
    float refDisplayOffset; // x axis
    float refDisplayScale; // x axis
};