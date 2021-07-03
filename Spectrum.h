#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class TimeSlider;

class Spectrum
{
public:
    Spectrum(int nFreqs, float noteFreq, float maxFreq, int nKeyFrames, float duration);

    // Logarithmically scale frequencies to ensure less freqs within lower range of human hearing.
    float getFrequency(int index);
    void setFirstFrequency(float freq);
    int getNFreqs();

    void setMagnitude(int fIndex, float mag);
    float getMagnitude(int fIndex);

    void setTime(float t);
    float getTime();

    float getDuration();
    int getNKeyFrames();

    TimeSlider* timeSlider = nullptr;
    void updateKeyFrameTimes(juce::Array<float>& arrayOfKFTimes);

    enum PlayState {EditingSpectrum, PlayingSound, Stopped};
    PlayState getPlayState();
    void setPlayState(PlayState pS);

private:

    class KeyFrame
    {
    public:
        KeyFrame(int index, Spectrum* spec);
        
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
        const Spectrum* spectrum;

        inline float interpolate(int fIndex, KeyFrame* left, KeyFrame* right);
    };

    float noteFreq;
    
    const float maxFreq;
    const float freqRange;
    const int nKeyFrames;
    const int nFreqs;
    const float duration; // in seconds
    
    juce::OwnedArray<KeyFrame> keyFrames;
    
    int keyFrameIndex;
    float time;

    PlayState playState;
    
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