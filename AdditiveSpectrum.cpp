#include <juce_gui_extra/juce_gui_extra.h>

#include "AdditiveSpectrum.h"
#include "TimeSlider.h"

AdditiveSpectrum::AdditiveSpectrum(int nFreqs, float noteFreq, float duration, int nKeyFrames)
        : Spectrum(nFreqs, duration), noteFreq(noteFreq),
          nKeyFrames(nKeyFrames), keyFrameIndex(0)
{
    // Initialise Default Keyframes of empty magnitude
    for (auto i=0; i<nKeyFrames; i++)
    {
        keyFrames.add(new KeyFrame(i, this));
    }

    // Set first keyframe to active, initialising its spectrum to all 0s:
    keyFrames[keyFrameIndex]->setActive(); // Also sets key frame active links
}

// Logarithmically scale frequencies to ensure less freqs within lower range of human hearing.
float AdditiveSpectrum::getFrequency(int freqIndex)
{
    //TODO pow(2.0, (inputFreq - noteFreq));
    return noteFreq * (float) (freqIndex+1);
}

void AdditiveSpectrum::setFirstFrequency(float freq)
{
    noteFreq = freq;
}

void AdditiveSpectrum::setMagnitude(int fIndex, float mag)
{
    if (playState != PlayingSound)
    {
        playState = EditingSpectrum;
        if(keyFrameExists(keyFrameIndex))
            keyFrames[keyFrameIndex]->setMagnitude(fIndex, mag);
    }
}

float AdditiveSpectrum::getMagnitude(int fIndex)
{
    if(keyFrameExists(keyFrameIndex))
        return keyFrames[keyFrameIndex]->getMagnitude(fIndex);
    return 0.0f;
}

void AdditiveSpectrum::setTime(float t)
{
    if (playState == Stopped) playState = EditingSpectrum;
    time = t;
    keyFrameIndex = juce::roundToInt<float>((time / duration)*(nKeyFrames-1));
}

void AdditiveSpectrum::updateKeyFrameTimes(juce::Array<float>& arrayOfKFTimes)
{
    arrayOfKFTimes.clear();
    KeyFrame* kF = keyFrames[0];

    while (kF != nullptr)
    {
        if (kF->getIsActive()) arrayOfKFTimes.add(kF->getTimeStamp());
        kF = kF->getNextActive();
    }

}
void AdditiveSpectrum::deleteKeyframe(float t)
{
    int kFIndex = juce::roundToInt<float>((t / duration)*(nKeyFrames-1));
    if(keyFrameExists(kFIndex))
        keyFrames[kFIndex]->removeActive();
}

int AdditiveSpectrum::getNKeyFrames() {return nKeyFrames;}






AdditiveSpectrum::KeyFrame::KeyFrame(int index, AdditiveSpectrum* spec)
        : timeStamp(((float) index / (spec->nKeyFrames-1)) * spec->duration),
            kFIndex(index),
            isActive(false),
            nextActive(nullptr),
            prevActive(nullptr),
            spectrum(spec) {}
    
bool AdditiveSpectrum::KeyFrame::operator<(KeyFrame a) {return kFIndex < a.kFIndex;}

AdditiveSpectrum::KeyFrame* AdditiveSpectrum::KeyFrame::getNextActive() {return nextActive;}
float AdditiveSpectrum::KeyFrame::getTimeStamp() {return timeStamp;}
    
void AdditiveSpectrum::KeyFrame::setMagnitude(int fIndex, float mag)
{
    if (!isActive) setActive(prevActive);
    magnitudes.set(fIndex, mag);
}
    
float AdditiveSpectrum::KeyFrame::getMagnitude(int fIndex)
{
    bool beforeKF = (spectrum->time < timeStamp);
    KeyFrame* left;
    KeyFrame* right;
    if (isActive) // this key frame is active
    {
        if (beforeKF) // time point is before this key frame
        {
            right = this;
            left = prevActive;
        }
        else // time point is after this key frame
        {
            left = this;
            right = nextActive;
        }
    }
    else // this key frame is not active - use known surrounding active keyframes.
    {
        left = prevActive;
        right = nextActive;
    }

    if (!left && !right) // If there is not an active keyframe on either side, return this keyframe's value
        return magnitudes[fIndex];
    else if (left && !right)
        return left->magnitudes[fIndex];
    else if (!left && right)
        return right->magnitudes[fIndex];
    
    // Interpolate:
    return interpolate(fIndex, left, right);
}

void AdditiveSpectrum::KeyFrame::setActive(KeyFrame* toCopy)
{
    if (magnitudes.size() > 0) magnitudes.clear(); // Reset keyframe if necessary
    for (auto i = 0; i < spectrum->nFreqs; i++)
    {
        if (toCopy == nullptr) magnitudes.add(0.0f);
        else magnitudes.add(toCopy->getMagnitude(i));
    }
    isActive = true;

    // Must refresh keyframe active links after setting a new keyframe to active:
    refreshKFLinks();
    // Refresh GUI to include keyframe markers:
    if (spectrum && spectrum->timeSlider) spectrum->timeSlider->refreshKeyFrameMarkers();
}

void AdditiveSpectrum::KeyFrame::removeActive()
{
    if (isActive)
    {
        magnitudes.clear();
        isActive = false;
        refreshKFLinks();
        if (spectrum && spectrum->timeSlider) spectrum->timeSlider->refreshKeyFrameMarkers();
    }
}

bool AdditiveSpectrum::KeyFrame::getIsActive() {return isActive;}

void AdditiveSpectrum::KeyFrame::refreshKFLinks()
{
    KeyFrame* tempActiveKF = nullptr;
    KeyFrame* kF= nullptr;

    // Set prevActive s
    for (auto i = 0; i < spectrum->nKeyFrames; i++)
    {
        kF = spectrum->keyFrames[i];
        kF->prevActive = nullptr;

        if (tempActiveKF) kF->prevActive = tempActiveKF;
        if (kF->isActive) tempActiveKF = kF;
    }
    // Set nextActive s
    tempActiveKF = nullptr;
    for (auto i = spectrum->nKeyFrames - 1; i >= 0; i--)
    {
        kF = spectrum->keyFrames[i];
        kF->nextActive = nullptr;

        if (tempActiveKF) kF->nextActive = tempActiveKF;
        if (kF->isActive) tempActiveKF = kF;
    }
}

inline float AdditiveSpectrum::KeyFrame::interpolate(int fIndex, KeyFrame* left, KeyFrame* right)
{
    // Linear
    float leftValue = left->magnitudes[fIndex];
    float rightValue = right->magnitudes[fIndex];
    float leftTime = left->timeStamp;
    float rightTime = right->timeStamp;
    float slope = (rightValue - leftValue) / (rightTime - leftTime);
    float deltaTime = spectrum->time - leftTime;
    return leftValue + slope * deltaTime;
}