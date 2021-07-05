#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class Spectrum;
class AdditiveSpectrum;
class FFTSpectrum;

class SpectrumEditor : public juce::Component
{
public:
    SpectrumEditor(AdditiveSpectrum& spectrum, FFTSpectrum& refSpectrum);

    void paint (juce::Graphics& g) override;

    void mouseUp (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;

    void refreshPoints();
    void refreshRefPoints();

    void clearSpectrum();
    void clearRefSpectrum();

    void addRefSpectrum();
    
private:
    struct SpectrumPoint
    {
        SpectrumPoint(int i, Spectrum& spectrum);
        float magnitude;
        juce::Point<float> displayCoords;
        bool selected = false;
        const int index;
        Spectrum& spectrum;

        inline void updateMagnitude(float mag);
        inline void fromSpectrum();
        inline float getFrequency();
    };
    
    juce::OwnedArray<SpectrumPoint> spectrumPoints;
    juce::OwnedArray<SpectrumPoint> refSpectrumPoints;
    inline float coordsToMagnitude(const juce::Point<float>& point);
    inline void updateDisplayCoords(SpectrumPoint* point);
    
    // Point selected:
    SpectrumPoint* pointSelected = nullptr;

    const float topPadding = 5.0f;
    const float bottomPadding = 10.0f;
    const float leftPadding = 7.0f;
    const float boundaryRadius = 5.0f;
    inline bool inBoundary(const juce::Point<float>& circleCenter, const juce::Point<float>& testPoint);
    
    AdditiveSpectrum& spectrum;
    FFTSpectrum& refSpectrum;
};