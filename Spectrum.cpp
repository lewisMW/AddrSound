#include "Spectrum.h"

Spectrum::Spectrum(int nFreqs, float duration)
        : nFreqs(nFreqs), duration(duration), time(0.0f),
          iterFreqIndex(0), iterMagIndex(0) {}


int Spectrum::getNFreqs() {return nFreqs;}
float Spectrum::getTime() {return time;}
float Spectrum::getDuration() {return duration;}
Spectrum::PlayState Spectrum::getPlayState() {return playState;}
void Spectrum::setPlayState(Spectrum::PlayState pS) {playState = pS;}

void Spectrum::setTime(float t)
{
    time = t;
}

float Spectrum::nextFrequency()
{
    float freq = getFrequency(iterFreqIndex++);
    if (iterFreqIndex >= nFreqs) iterFreqIndex = 0;
    return freq;
}

float Spectrum::nextMagnitude()
{
    float mag = getMagnitude(iterMagIndex++);
    if (iterMagIndex >= nFreqs) iterMagIndex = 0;
    return mag;
}
