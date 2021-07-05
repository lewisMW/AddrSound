#include "Spectrum.h"

Spectrum::Spectrum(int nFreqs, float duration)
        : nFreqs(nFreqs), duration(duration), time(0.0f) {}


int Spectrum::getNFreqs() {return nFreqs;}
float Spectrum::getTime() {return time;}
float Spectrum::getDuration() {return duration;}
Spectrum::PlayState Spectrum::getPlayState() {return playState;}
void Spectrum::setPlayState(Spectrum::PlayState pS) {playState = pS;}

void Spectrum::setTime(float t)
{
    time = t;
}