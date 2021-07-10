#include <juce_gui_extra/juce_gui_extra.h>

#include "AdditiveSpectrum.h"
#include "TimeSlider.h"

AdditiveSpectrum::AdditiveSpectrum(int nFreqs, float noteFreq, float duration, int nKeyFrames)
        : Spectrum(nFreqs, duration), noteFreq(noteFreq),
          nKeyFrames(nKeyFrames), keyFrameIndex(0), copiedKeyFrame(nullptr),
          timeFloatEpsilon(duration/(2*4*(nKeyFrames)))
{
    jassert(nKeyFrames > 0);

    // Initialise Default Keyframes of empty magnitude
    for (auto i=0; i<nKeyFrames; i++)
    {
        keyFrames.add(new KeyFrame(i, this));
    }

    // Set first keyframe to active, initialising its spectrum to all 0s:
    keyFrames[keyFrameIndex]->setActive();
    keyFrames[keyFrameIndex]->refreshKFLinks();
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
        auto kf = keyFrames[keyFrameIndex];
        if(keyFrameExists(keyFrameIndex))
        {
            if (!kf->getIsActive())
            {
                kf->copyFrom(kf->getPrevActive(), time);
                kf->setActive();
                kf->refreshKFLinks();
            }
            kf->setMagnitude(fIndex, mag);
        }
    }
}

float AdditiveSpectrum::getMagnitude(int fIndex)
{
    if(keyFrameExists(keyFrameIndex))
        return keyFrames[keyFrameIndex]->getMagnitude(fIndex, time);
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

void AdditiveSpectrum::copyKeyFrame()
{
    copiedKeyFrame = keyFrames[keyFrameIndex];
}

void AdditiveSpectrum::pasteKeyFrame()
{
    if (copiedKeyFrame && keyFrameExists(keyFrameIndex))
    {
        auto kf = keyFrames[keyFrameIndex];
        kf->copyFrom(copiedKeyFrame, copiedKeyFrame->getTimeStamp());
        if (! kf->getIsActive())
        {
            kf->setActive();
            kf->refreshKFLinks();
        }
    }
}

int AdditiveSpectrum::getNKeyFrames() {return nKeyFrames;}



void AdditiveSpectrum::saveSpectrum(juce::FileOutputStream& outputStream)
{
    juce::ValueTree fileDataTree("SpectrumData");
    // Metadata:
    fileDataTree.setProperty("nKeyFrames", nKeyFrames, nullptr);
    fileDataTree.setProperty("nFreqs", nFreqs, nullptr);
    fileDataTree.setProperty("duration", duration, nullptr);
    fileDataTree.setProperty("noteFreq", noteFreq, nullptr);
    
    // Keyframes:
    juce::ValueTree activeKeyframeTree("KeyframeData");
    KeyFrame* kF = keyFrames[0];
    while (kF != nullptr)
    {
        if (kF->getIsActive())
        {
            juce::ValueTree keyframeData("Keyframe");
            keyframeData.setProperty("kFIndex", kF->getIndex(), nullptr);
            for (int i=0; i < nFreqs; i++)
            {
                float magnitude = kF->getMagnitude(i, 0.0f, true);
                if (magnitude == 0.0f) continue; // only save non-trivial points

                juce::ValueTree magnitudeData("magnitude");
                magnitudeData.setProperty("index", i, nullptr);
                magnitudeData.setProperty("value", magnitude, nullptr);
                keyframeData.addChild(magnitudeData,-1,nullptr);
            }
            activeKeyframeTree.addChild(keyframeData, -1, nullptr);
        }
        kF = kF->getNextActive();
    }
    fileDataTree.addChild(activeKeyframeTree, -1, nullptr);

    fileDataTree.writeToStream(outputStream);
}
void AdditiveSpectrum::loadSpectrum(juce::FileInputStream& inputStream)
{
    setTime(0.0f); // Reset time to the first 0th keyframe. Audio thread should thus not perform interpolation no matter how many keyframes are added/removed (Not guaranteed!)

    juce::ValueTree fileDataTree = juce::ValueTree::readFromStream(inputStream);

    nKeyFrames = fileDataTree["nKeyFrames"];
    nFreqs = fileDataTree["nFreqs"];
    duration = fileDataTree["duration"];
    noteFreq = fileDataTree["noteFreq"];
    timeFloatEpsilon = duration/(2*4*(nKeyFrames));
    
    jassert(nKeyFrames > 0 && nFreqs > 0 && duration > 0.0f);
    
    
    for (auto i=0; i < nKeyFrames; i++)
    {   
        if (i < keyFrames.size()) // Reset data of current keyframes:
            keyFrames[i]->reset();
        else // Allocate more keyframes if more are needed:
            keyFrames.add(new KeyFrame(i, this));
    }
    
    // Load active keyframes:
    juce::ValueTree activeKeyframeTree = fileDataTree.getChildWithName("KeyframeData");
    for (juce::ValueTree keyframeData : activeKeyframeTree)
    {
        int kFIndex = keyframeData["kFIndex"];
        KeyFrame* kf = keyFrames[kFIndex];
        kf->setActive(); // Set keyframe as active
        for (juce::ValueTree magnitudeData : keyframeData)
        {
            int i = magnitudeData["index"];
            kf->setMagnitude(i, magnitudeData["value"]);
        }
    }
    keyFrames[0]->refreshKFLinks();
}















AdditiveSpectrum::KeyFrame::KeyFrame(int index, AdditiveSpectrum* spec)
        : timeStamp(((float) index / (spec->nKeyFrames-1)) * spec->duration),
            kFIndex(index),
            isActive(false),
            nextActive(nullptr),
            prevActive(nullptr),
            spectrum(spec)
{
    for (auto i = 0; i < spectrum->nFreqs; i++)
    {
        magnitudes.add(0.0f);
    }
}

void AdditiveSpectrum::KeyFrame::reset()
{
    isActive = false;

    for (auto i = 0; i < spectrum->nFreqs; i++)
    {
        if (i < magnitudes.size())
            magnitudes.set(i, 0.0f);
        else
            magnitudes.add(0.0f);
    }

    timeStamp = (float) kFIndex / (spectrum->nKeyFrames-1) * spectrum->duration;
    nextActive = nullptr;
    prevActive = nullptr;
}
    
bool AdditiveSpectrum::KeyFrame::operator<(KeyFrame a) {return kFIndex < a.kFIndex;}

AdditiveSpectrum::KeyFrame* AdditiveSpectrum::KeyFrame::getNextActive() {return nextActive;}
AdditiveSpectrum::KeyFrame* AdditiveSpectrum::KeyFrame::getPrevActive() {return prevActive;}
float AdditiveSpectrum::KeyFrame::getTimeStamp() {return timeStamp;}
int AdditiveSpectrum::KeyFrame::getIndex() {return kFIndex;}
    
void AdditiveSpectrum::KeyFrame::setMagnitude(int fIndex, float mag)
{
    magnitudes.set(fIndex, mag);
}
    
float AdditiveSpectrum::KeyFrame::getMagnitude(int fIndex, float atTime, bool trueValue)
{
    if (trueValue) return magnitudes[fIndex]; // return true value without interpolation
    
    bool beforeKF = (atTime < timeStamp);
    KeyFrame* left;
    KeyFrame* right;
    if (isActive) // this key frame is active
    {
        // If the time cursor is very close to this keyframe, just return this keyframe's value
        if (std::abs(atTime - timeStamp) < spectrum->timeFloatEpsilon) return magnitudes[fIndex];

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
    return interpolate(fIndex, atTime, left, right);
}

void AdditiveSpectrum::KeyFrame::copyFrom(KeyFrame* toCopy, float t)
{
    for (auto i = 0; i < spectrum->nFreqs; i++)
    {
        if (toCopy == nullptr) magnitudes.set(i, 0.0f);
        else magnitudes.set(i, toCopy->getMagnitude(i, t));
    }
}

void AdditiveSpectrum::KeyFrame::setActive()
{
    isActive = true;
}

void AdditiveSpectrum::KeyFrame::removeActive()
{
    if (isActive)
    {
        for (float& val : magnitudes) val = 0.0f;
        isActive = false;
        refreshKFLinks();
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

    // Refresh GUI to include keyframe markers:
    if (spectrum && spectrum->timeSlider) spectrum->timeSlider->refreshKeyFrameMarkers();
}

inline float AdditiveSpectrum::KeyFrame::interpolate(int fIndex, float atTime, KeyFrame* left, KeyFrame* right)
{
    // Linear
    float leftValue = left->magnitudes[fIndex];
    float rightValue = right->magnitudes[fIndex];
    float leftTime = left->timeStamp;
    float rightTime = right->timeStamp;
    float slope = (rightValue - leftValue) / (rightTime - leftTime);
    float deltaTime = atTime - leftTime;
    return leftValue + slope * deltaTime;
}