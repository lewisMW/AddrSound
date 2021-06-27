#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : spectrum(50, 40, 1000), spectrumEditor(spectrum)
{
    createWaveTable();

    setSize (600, 400);
    setAudioChannels(0,2); // Only outputs

    setWantsKeyboardFocus(true);
    addKeyListener(this);

    addAndMakeVisible(spectrumEditor);
    addMouseListener(&spectrumEditor, false);
}
MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================

void MainComponent::createWaveTable()
{
    sineTable.setSize(1, (int) tableSize + 1);
    auto* samples = sineTable.getWritePointer(0);
    auto angleDelta = juce::MathConstants<double>::twoPi / (double) (tableSize-1);
    auto currentAngle = 0.0;

    for (unsigned int i=0; i < tableSize; i++)
    {
        auto sample = std::sin(currentAngle);
        samples[i] = (float) sample;
        currentAngle += angleDelta;
    }
    samples[tableSize] = samples[0];
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    for(auto i=0; i < spectrum.size; i++)
    {
        auto* oscillator = new WavetableOscillator(sineTable, (float) sampleRate);
        oscillator->setFrequency(spectrum.freqs[i]);
        oscillator->setAmplitude(spectrum.magnitudes[i]);
        oscillators.add(oscillator);
    }
    level = 0.25f / (float) spectrum.size;
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* leftBuffer = bufferToFill.buffer->getWritePointer(0,bufferToFill.startSample);
    auto* rightBuffer = bufferToFill.buffer->getWritePointer(1,bufferToFill.startSample);
    bufferToFill.clearActiveBufferRegion();

    for(auto oIndex=0; oIndex < oscillators.size(); oIndex++)
    {
        auto* oscillator = oscillators.getUnchecked(oIndex);
        oscillator->setAmplitude(spectrum.magnitudes[oIndex]);
        oscillator->setFrequency(spectrum.freqs[oIndex]);

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
}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.

    spectrumEditor.setBounds(50,50,400,300);
}

//==============================================================================
bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    auto keyCode = key.getKeyCode();
    auto noteOffset = keyCode - 49;
    DBG(std::to_string(keyCode));

    for(auto oIndex=0; oIndex < oscillators.size(); oIndex++)
    {
        auto* oscillator = oscillators.getUnchecked(oIndex);
        
        auto midiNote = 69 + noteOffset;
        auto freq = 440.0*pow(2.0, (midiNote-69.0)/12.0);
        oscillator->setFrequency((float) freq);
    }
    return true;
}