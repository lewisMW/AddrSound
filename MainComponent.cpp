#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    createWaveTable();

    setSize (600, 400);
    setAudioChannels(0,2); // Only outputs

    addAndMakeVisible(freqSlider);
    freqSlider.setRange(50.0,5000.0);
    freqSlider.setSkewFactorFromMidPoint(500.0);
}
MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================

void MainComponent::createWaveTable()
{
    sineTable.setSize(1, (int) tableSize);
    auto* samples = sineTable.getWritePointer(0);
    auto angleDelta = juce::MathConstants<double>::twoPi / (double) (tableSize-1);
    auto currentAngle = 0.0;

    for (unsigned int i=0; i < tableSize; i++)
    {
        auto sample = std::sin(currentAngle);
        samples[i] = (float) sample;
        currentAngle += angleDelta;
    }
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    auto numberOfOscillators = 6;

    for(auto i=0; i < numberOfOscillators; i++)
    {
        auto* oscillator = new WavetableOscillator(sineTable);
        auto midiNote = 69 + 5*i;
        auto freq = 440.0*pow(2.0, (midiNote-69.0)/12.0);
        oscillator->setFrequency((float) freq, (float) sampleRate);
        oscillators.add(oscillator);
    }
    level = 0.25f / (float) numberOfOscillators;
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* leftBuffer = bufferToFill.buffer->getWritePointer(0,bufferToFill.startSample);
    auto* rightBuffer = bufferToFill.buffer->getWritePointer(1,bufferToFill.startSample);
    bufferToFill.clearActiveBufferRegion();

    for(auto oIndex=0; oIndex < oscillators.size(); oIndex++)
    {
        auto* oscillator = oscillators.getUnchecked(oIndex);

        for (auto sample = 0; sample < bufferToFill.numSamples; sample++)
        {
            auto current = oscillator->getNextSample() * level;
            leftBuffer[sample] += current;
            rightBuffer[sample] += current;
        }
    }
}


void MainComponent::releaseResources()
{
    juce::Logger::getCurrentLogger()->writeToLog ("Releasing audio resources");
}


//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::Font (16.0f));
    g.setColour (juce::Colours::white);
    g.drawText ("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.

    freqSlider.setBounds(10,20,getWidth()-10,20);

}
