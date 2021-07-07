#include "FFTSpectrum.h"

FFTSpectrum::FFTSpectrum(int nFq, int windowSamples, int downSamplingRate)
    : Spectrum(nFq,  0.0f), fftWindowSampleN(windowSamples), audioSource(nullptr), downSamplingRate(downSamplingRate)
{
    fftSpectrumArray.resize(nFreqs*2); // to account for other half of the symmetric FFT
    fftSpectrumArrayAbs.resize(nFreqs);
    
    inputBufferSize = fftWindowSampleN * downSamplingRate; // Take extra audio file samples for later downsampling
    
    inputBufferInfo.numSamples = inputBufferSize;
    inputBufferInfo.startSample = 0;
    inputBufferInfo.buffer = new juce::AudioSampleBuffer(2, inputBufferSize);
}

FFTSpectrum::~FFTSpectrum()
{
    delete inputBufferInfo.buffer;
    removeAudioSource();
}

float FFTSpectrum::getFrequency(int index) 
{
    auto nFFTPoints = nFreqs*2;
    return index * (fs/downSamplingRate) / nFFTPoints;
}
float FFTSpectrum::getMagnitude(int fIndex) 
{
    return fftSpectrumArrayAbs[fIndex];
}

float FFTSpectrum::getWindowPeriodSeconds() {return (float) inputBufferSize / fs;}

juce::AudioSourceChannelInfo* FFTSpectrum::getInputBufferContainer() {return &inputBufferInfo;}

void FFTSpectrum::addAudioSource(juce::AudioFormatReader* reader, bool ownsReader)
{
    audioSource.reset(new juce::AudioFormatReaderSource(reader, ownsReader));
    fs = (float) audioSource->getAudioFormatReader()->sampleRate;
    nTotalSamples = audioSource->getAudioFormatReader()->lengthInSamples;
    duration = (float) nTotalSamples / fs;

    audioSource->prepareToPlay(fftWindowSampleN, (double) fs);
}
void FFTSpectrum::removeAudioSource()
{
    if (audioSource.get()) audioSource->releaseResources();
    playState = Stopped;
    audioSource = nullptr;
}

void FFTSpectrum::setTime(float t) 
{
    juce::int64 sampleIndex = (juce::int64) (t * fs);
    audioSource->setNextReadPosition(sampleIndex);
}

void FFTSpectrum::refreshFFT()
{
    audioSource->getNextAudioBlock(inputBufferInfo);
    auto* leftBuffer = inputBufferInfo.buffer->getReadPointer(0);

    juce::Array<float> downSampled;
    downSampled.resize(fftWindowSampleN);
    // skip every 2nd point:
    for (int i=0; i < fftWindowSampleN; i++) // fftWindowSampleN = inputBufferSize/downSamplingRate
        downSampled.set(i, *(leftBuffer + downSamplingRate*i) );

    auto nFFTPoints = nFreqs*2; // 2x DFT points to account for halving the symmetric spectrum
    auto* spectrumArr = fftSpectrumArray.getRawDataPointer();
    auto* downSampledSignal = downSampled.getRawDataPointer();

    hanningWindow(downSampledSignal, fftWindowSampleN); // Apply a Hanning window

    FFT(downSampledSignal, spectrumArr, nFFTPoints, fftWindowSampleN); 

    float max = 0.0f;
    // Absolute value
    for (int i=0; i<nFreqs; i++)
    {
        float abs = std::abs<float>(fftSpectrumArray[i]);
        fftSpectrumArrayAbs.set(i,abs);
        if (abs > max) max = abs;
    }

    // Normalise
    if (max > 0.0f)
    {
        for (float& x : fftSpectrumArrayAbs) x /= max;
    }
}

void FFTSpectrum::hanningWindow(float* x, int N)
{
    const float pi = std::acosf(-1);
    // Windowing : (lecture 7, p55)
    auto hanning = [&](int n){
        return 0.5f * (1 + std::cosf(pi * (n - N/2) / (N/2 + 1) ) );
    };
    for (int i=0; i<N; i++) x[i] *= hanning(i);
}



void FFTSpectrum::FFT(const float* x_raw, std::complex<float>* X, int nDFTSamples, int nSignalSamples)
{
    using namespace std::complex_literals;
    
    const double pi = std::acos(-1); // shortcut to get pi
    static const std::complex<double> twoPi = 2*pi;
    int nBits = (int) std::log2f((float) nDFTSamples);

    // Bit-reversed decimal:
    auto bitReverse = [nBits] (int d) {
        int result = 0;
        for (int bit=0; bit < nBits; bit++)
        {
            int shiftAmount = (nBits-1-2*bit);
            if (shiftAmount > 0)
                result |= ((d & (1 << bit)) << shiftAmount);
            else
                result |= ((d & (1 << bit)) >> -shiftAmount);
        }
        return result;
    };
    std::vector<int> bitReversedIndexs(nDFTSamples);
    for (int i=0; i < bitReversedIndexs.size(); i++) bitReversedIndexs[i] = bitReverse(i);

    // Zero-padding:
    std::vector<float> x_padded(nDFTSamples);
    float* x = x_padded.data();
    if (nSignalSamples > nDFTSamples) nSignalSamples = nDFTSamples;
    std::memcpy(x, x_raw, sizeof(float) * nSignalSamples);
    std::memset(x + nSignalSamples, 0, sizeof(float)*(nDFTSamples-nSignalSamples));

    // Decimation-in-time DIT from lecture 7 p26 :
    struct stackData {
        stackData(int k, int N) : k(k), N(N), W_N(std::exp(-1i * (twoPi/std::complex<double>(N)) )) {}
        int k;
        int N;
        std::complex<double> W_N; // Twiddle factor
        int step = 0;
    };
    std::stack<stackData> stack;

    stack.push(stackData(0, nDFTSamples)); // First frame
    while (!stack.empty())
    {
        int N = stack.top().N;
        int k = stack.top().k;
        auto& W_N = stack.top().W_N;
        int& step = stack.top().step;
        if (N > 2)
        {
            switch (step)
            {
                case 0:
                    stack.top().step++;
                    stack.push(stackData(k, N/2));
                    break;
                case 1:
                    stack.top().step++;
                    stack.push(stackData(k+N/2, N/2));
                    break;
                case 2:
                    for (int i=0; i < N/2; i++)
                    {
                        std::complex<float> X_e = X[k+i];
                        std::complex<float> X_o = X[k+N/2+i];
                        std::complex<float> W_N_k; //W_N^k twiddle factor
                        std::complex<float> WNk_Xo; //W_N^k*X_o twiddle factor times X_o

                        // Calculate twiddle factor:
                        if (X_o == 0.0f) // Optimise performance by skipping complex exponential calculation (esp. for zero-padding values)
                            WNk_Xo = 0.0f;
                        else
                        {
                            if (i == 0) W_N_k = 1.0f; // Optimise performance as e^0 == 1
                            else W_N_k = (std::complex<float>) std::pow(W_N, i);
                            WNk_Xo = W_N_k * X_o; 
                        }
                        
                        X[k+i] = X_e + WNk_Xo;
                        X[k+N/2+i] = X_e - WNk_Xo;
                    }
                    stack.pop();
            }          
        }
        else
        {
            const float* x_e = x + bitReversedIndexs[k];
            const float* x_o = x + bitReversedIndexs[k+N/2];

            auto W_N_k = 1.0f; // i is 0
            auto WNk_xo = W_N_k * (*x_o); //TODO if *x_o == 0, 0
            X[k] = *x_e + WNk_xo;
            X[k+1] = *x_e - WNk_xo;
            stack.pop();
        }
    }
}