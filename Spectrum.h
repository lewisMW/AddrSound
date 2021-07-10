#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class Spectrum
{
public:
    Spectrum(int nFreqs, float duration);

    virtual float getFrequency(int index) = 0;
    int getNFreqs();
    virtual float getMagnitude(int fIndex) = 0;
    virtual void setMagnitude(int /*fIndex*/, float /*mag*/) {}

    virtual float nextFrequency();
    virtual float nextMagnitude();

    virtual void setTime(float t);
    float getTime();

    float getDuration();

    enum PlayState {EditingSpectrum, PlayingSound, Stopped};
    PlayState getPlayState();
    void setPlayState(PlayState pS);

    struct Peaks
    {
        std::vector<int> indexs;
        std::vector<float> values;
    };

protected:

    int nFreqs;
    float duration; // in seconds
    float time;

    int iterFreqIndex;
    int iterMagIndex;

    PlayState playState;
};