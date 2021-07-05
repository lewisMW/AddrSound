#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "Spectrum.h"

class TimeSlider;

class AdditiveSpectrum : public Spectrum
{
public:
    AdditiveSpectrum(int nFreqs, float noteFreq, float duration, int nKeyFrames);

    // Logarithmically scale frequencies to ensure less freqs within lower range of human hearing.
    float getFrequency(int index) override;
    void setFirstFrequency(float freq);

    void setMagnitude(int fIndex, float mag) override;
    float getMagnitude(int fIndex) override;

    void setTime(float t) override;

    int getNKeyFrames();

    TimeSlider* timeSlider = nullptr;
    void updateKeyFrameTimes(juce::Array<float>& arrayOfKFTimes);
    void deleteKeyframe(float t);

private:

    class KeyFrame
    {
    public:
        KeyFrame(int index, AdditiveSpectrum* spec);
        
        bool operator<(KeyFrame a);

        KeyFrame* getNextActive();
        float getTimeStamp();
        
        void setMagnitude(int fIndex, float mag);
        float getMagnitude(int fIndex);
        
        void setActive(KeyFrame* toCopy = nullptr);
        void removeActive();
        bool getIsActive();

        void refreshKFLinks();

    private:
        juce::Array<float> magnitudes;
        bool isActive;
        const int kFIndex;
        const float timeStamp;
        KeyFrame* nextActive;
        KeyFrame* prevActive;
        const AdditiveSpectrum* spectrum;

        inline float interpolate(int fIndex, KeyFrame* left, KeyFrame* right);
    };
    
    const int nKeyFrames;
    juce::OwnedArray<KeyFrame> keyFrames;
    int keyFrameIndex;

    float noteFreq;
    
    enum ErrorCode {KeyFrameOutOfBounds, NullKeyFrame, KeyFrameExists};
    inline void printError(int kFIndex, ErrorCode err)
    {
        switch(err)
        {
            case KeyFrameOutOfBounds:
                DBG("Keyframe out of bounds! Given index=" + juce::String(kFIndex) + " > nKeyFrames = " + juce::String(nKeyFrames));
                break;
            case NullKeyFrame:
                DBG("Keyframe does not exist at given index=" + juce::String(kFIndex));
                break;
            case KeyFrameExists:
                DBG("Keyframe already exists at index=" + juce::String(kFIndex));
                break;
        }
    }
    inline int keyFrameExists(int kFIndex, bool checkExists=true)
    {
        if (kFIndex >= nKeyFrames)
        {
            printError(kFIndex, KeyFrameOutOfBounds);
            return -1;
        }
        if (keyFrames[kFIndex] == nullptr && checkExists)
        {
            printError(kFIndex, NullKeyFrame);
            return -1;
        }
        if (keyFrames[kFIndex] != nullptr && !checkExists)
        {
            printError(kFIndex, KeyFrameExists);
            return -1;
        }
        return 1;
    }
};