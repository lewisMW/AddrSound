#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize (600, 400);
    setAudioChannels(0,2);

    addAndMakeVisible(freqSlider);
    freqSlider.setRange(50.0,5000.0);
    freqSlider.setSkewFactorFromMidPoint(500.0);
    freqSlider.onValueChange = [this] {updateOmega();};
}
MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    fs = sampleRate;
    updateOmega();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto level = 0.125f;
    auto* leftBuffer = bufferToFill.buffer->getWritePointer(0,bufferToFill.startSample);
    auto* rightBuffer = bufferToFill.buffer->getWritePointer(1,bufferToFill.startSample);
    
    for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
    {
        auto current = (float) std::sin(currentAngle);
        currentAngle += omega_0;
        leftBuffer[sample] = current*level;
        rightBuffer[sample] = current*level;
    }
}

void MainComponent::releaseResources()
{
    juce::Logger::getCurrentLogger()->writeToLog ("Releasing audio resources");
}

void MainComponent::updateOmega() {
    if (fs > 0.0)
        omega_0 = freqSlider.getValue() * 2.0 * juce::MathConstants<double>::pi / fs;
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
