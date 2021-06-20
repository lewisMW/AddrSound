#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize (600, 400);
    setAudioChannels(0,2);
}
MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    juce::String message;
    message << "Preparing to play audio...\n";
    message << " samplesPerBlockExpected = " << samplesPerBlockExpected << "\n";
    message << " sampleRate = " << sampleRate;
    juce::Logger::getCurrentLogger()->writeToLog (message);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    for (auto channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
    {
        // Get a pointer to the start sample in the buffer for this audio output channel
        auto* buffer = bufferToFill.buffer->getWritePointer (channel, bufferToFill.startSample);

        // Fill the required number of samples with noise between -0.125 and +0.125
        for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
            buffer[sample] = random.nextFloat() * 0.25f - 0.125f;
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
}
