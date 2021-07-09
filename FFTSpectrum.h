#pragma once

// My FFT function purely depends on the C++ standard library :
#include <complex>
#include <cmath>
#include <stack>
#include <vector>

#include <juce_audio_utils/juce_audio_utils.h>
#include <PeakFinder.h>

#include "Spectrum.h"

class FFTSpectrum : public Spectrum
{
public:
    FFTSpectrum(int nFq, int windowSamples, int downSamplingRate);

    ~FFTSpectrum();

    float getFrequency(int index) override;
    float getMagnitude(int fIndex) override;

    float getWindowPeriodSeconds();
    
    juce::AudioSourceChannelInfo* getInputBufferContainer();

    void setTime(float t) override;

    void addAudioSource(juce::AudioFormatReader* reader, bool ownsReader);

    void removeAudioSource();

    void refreshFFT();

    struct FFTPeaks
    {
        std::vector<int> indexs;
        std::vector<float> values;
    };
    void calcPeaks(FFTPeaks& peaks);

private:
    void hanningWindow(float* x, int N);

    void FFT(const float* x_raw, std::complex<float>* X, int nDFTSamples, int nSignalSamples);

    int fftWindowSampleN; // Must be power of 2
    int downSamplingRate; // Used to restrict the spectrum below fs/2
    int inputBufferSize;
    float fs;

    juce::int64 nTotalSamples;
    std::unique_ptr<juce::AudioFormatReaderSource> audioSource;
    juce::AudioSourceChannelInfo inputBufferInfo;

    juce::Array<std::complex<float>> fftSpectrumArray;
    juce::Array<float> fftSpectrumArrayAbs;
    
};