#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : additiveSpectrum(30, 440, 0.5f, 10),
      refSpectrum(512, 512, 2),
      spectrumEditor(additiveSpectrum, refSpectrum),
      timeSlider(additiveSpectrum, spectrumEditor)
{
    createWaveTable();

    setSize (600, 425);
    setAudioChannels(0,2); // Only outputs

    setWantsKeyboardFocus(true);
    addKeyListener(this);

    addAndMakeVisible(spectrumEditor);
    addMouseListener(&spectrumEditor, false);
    addAndMakeVisible(timeSlider);
    addMouseListener(&timeSlider, false);

    addAndMakeVisible(toolsButton);
    toolsButton.onChange = [this] {toolsMenuSelect();};

    addAndMakeVisible(refAudioPositionSlider);
    refAudioPositionSlider.setEnabled(false);
    addMouseListener(&refAudioPositionSlider, false);

    setKeyboardNoteBindings();    
}

MainComponent::~MainComponent()
{
    refAudioPlayer.removeAudioSource();
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

void MainComponent::prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate)
{
    for(auto i=0; i < additiveSpectrum.getNFreqs(); i++)
    {
        auto* oscillator = new WavetableOscillator(sineTable, (float) sampleRate);
        oscillator->setAmplitude(additiveSpectrum.getMagnitude(i));
        oscillator->setFrequency(additiveSpectrum.getFrequency(i));
        oscillators.add(oscillator);
    }
    level = 0.5f / (float) additiveSpectrum.getNFreqs();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* leftBuffer = bufferToFill.buffer->getWritePointer(0,bufferToFill.startSample);
    auto* rightBuffer = bufferToFill.buffer->getWritePointer(1,bufferToFill.startSample);
    bufferToFill.clearActiveBufferRegion();

    int nOscillators = oscillators.size();

    std::shared_ptr<juce::AudioSampleBuffer> refBufferBackup;
    const float* ref = nullptr;
    int refSample = 0;

    if (refAudioPlayer.playing.load(std::memory_order_acquire) == true) // play reference audio
    {
        refBufferBackup = std::atomic_load(&refAudioPlayer.refBuffer); 
        ref = refBufferBackup->getReadPointer(0);
        refSample = refBufferBackup->getNumSamples() - refAudioPlayer.samplesToPlay;
        if (refSample < 0 || refAudioPlayer.samplesToPlay <= 0)
        {
            refAudioPlayer.samplesToPlay = refBufferBackup->getNumSamples();
            refSample = 0;
        }
        nOscillators = 1; // Only need one 'oscillator', i.e. sound source!
    }

    for(auto oIndex=0; oIndex < nOscillators; oIndex++)
    {
        auto* oscillator = oscillators.getUnchecked(oIndex);
        oscillator->setAmplitude(additiveSpectrum.getMagnitude(oIndex));
        oscillator->setFrequency(additiveSpectrum.getFrequency(oIndex));

        for (auto sample = 0; sample < bufferToFill.numSamples; sample++)
        {
            auto current = 0.0f; 
            if(ref != nullptr) // Playing reference source
            {
                if (refAudioPlayer.samplesToPlay <= 0)
                {
                    refAudioPlayer.samplesToPlay = refBufferBackup->getNumSamples();
                    refSample = 0;
                }
                
                current += ref[refSample++];
                refAudioPlayer.samplesToPlay--;
            }
            else // Playing additive oscillators
            {
                current += oscillator->getNextSample() * level;
            }
            leftBuffer[sample] += current;
            rightBuffer[sample] += current;
        }
    }

}

void MainComponent::RefAudioPlayer::play(int startSample)
{
    refPlaybackSource->setNextReadPosition(startSample);
    refPlaybackSource->getNextAudioBlock(refPlaybackSourceInfo);
    playing.store(true, std::memory_order_release);
}
void MainComponent::RefAudioPlayer::pause()
{
    playing.store(false, std::memory_order_release);
}

void MainComponent::RefAudioPlayer::addAudioSource(juce::AudioFormatReader* reader)
{
    playing.store(false, std::memory_order_release);
    refPlaybackSource.reset(new juce::AudioFormatReaderSource(reader, false));
    fs = (int) refPlaybackSource->getAudioFormatReader()->sampleRate;
    nSamples = refPlaybackSource->getAudioFormatReader()->lengthInSamples;
    duration = (float) nSamples / fs;

    int bufferSize = (int) (previewSeconds * fs); // not an attribute of RefAudioPlayer to avoid thread conflicts
    refPlaybackSource->prepareToPlay(bufferSize, (double) fs);
    std::shared_ptr<juce::AudioSampleBuffer> tempBufferPointer(new juce::AudioSampleBuffer(2, bufferSize));
    refPlaybackSourceInfo.buffer = tempBufferPointer.get();
    refPlaybackSourceInfo.numSamples = bufferSize;
    refPlaybackSourceInfo.startSample = 0;
    std::atomic_store(&refBuffer, tempBufferPointer);
}
void MainComponent::RefAudioPlayer::removeAudioSource()
{
    if (refPlaybackSource.get()) refPlaybackSource->releaseResources();
}
MainComponent::RefAudioPlayer::RefAudioPlayer()
    : playing(false), samplesToPlay(0), previewSeconds(0.2f) {}


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
    timeSlider.setBounds(50,360,400,20);
    toolsButton.setBounds(350, 10, 100, 30);
    refAudioPositionSlider.setBounds(50, 400, 400, 20);
}


// Tools Menu:
MainComponent::ToolsButton::ToolsButton()
    : juce::ComboBox("Tools")
{
    setText("Tools");
    addItem(juce::String("EQ"), ItemIDs::EQID);
    addItem(juce::String("Distortion"), ItemIDs::DistortionID);
    addItem(juce::String("Noise"), ItemIDs::NoiseID);
    addItem(juce::String("Reverb"), ItemIDs::ReverbID);
    addItem(juce::String("Load Reference File"), ItemIDs::LoadFileID);
}
void MainComponent::toolsMenuSelect()
{
    
    ToolsButton::ItemIDs id = (ToolsButton::ItemIDs) toolsButton.getSelectedId();
    switch (id)
    {
        case ToolsButton::ItemIDs::EQID:
            break;
        case ToolsButton::ItemIDs::DistortionID:
            break;
        case ToolsButton::ItemIDs::NoiseID:
            break;
        case ToolsButton::ItemIDs::ReverbID:
            break;
        case ToolsButton::ItemIDs::LoadFileID:   
            loadReferenceFile();
    }
    toolsButton.setText("Tools");
}
void MainComponent::loadReferenceFile()
{
    juce::FileChooser fC("Choose Reference Audio File");
    if (fC.browseForFileToOpen())
    {
        refAudioPositionSlider.setEnabled(false);
        juce::File file = fC.getResult();
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        juce::AudioFormatReader* reader = formatManager.createReaderFor(file);
        if (reader != nullptr)
        {
            refSpectrum.removeAudioSource();
            refSpectrum.addAudioSource(reader, true);
            refSpectrum.setTime(0.0f);
            refSpectrum.refreshFFT();
            spectrumEditor.addRefSpectrum();
            spectrumEditor.repaint();
            
            refAudioPlayer.removeAudioSource();
            refAudioPlayer.addAudioSource(reader);

            refAudioPositionSlider.setValue(0.0);
            refAudioPositionSlider.setRange(0.0, refSpectrum.getDuration(), refSpectrum.getWindowPeriodSeconds());
            refAudioPositionSlider.onValueChange = [this] {
                refSpectrum.setTime((float) refAudioPositionSlider.getValue());                
                refSpectrum.refreshFFT();
                spectrumEditor.refreshRefPoints();
                spectrumEditor.repaint();
                refAudioPlayer.play((int) (refAudioPositionSlider.getValue()*refAudioPlayer.fs));
            };
            refAudioPositionSlider.onDragEnd = [this] {
                refAudioPlayer.pause();
            };
            refAudioPositionSlider.setEnabled(true);
        }
        else
        {
            juce::AlertWindow::showMessageBox(juce::AlertWindow::AlertIconType::WarningIcon,
            "Error Reading Audio File",
                "The file selected (" + file.getFileName() + ") could not be read as an audio file.");
        }
    }
}

//==============================================================================
bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* /*originatingComponent*/)
{
    auto keyCode = key.getKeyCode();
    if (keyboardNoteBindings.contains(keyCode))
    {
        auto noteOffset = keyboardNoteBindings[keyCode];
        auto midiNote = 69 + noteOffset;
        auto freq = 440.0*pow(2.0, (midiNote-69.0)/12.0);
        additiveSpectrum.setFirstFrequency((float) freq);
        timeSlider.playSound();
    }
    return true;
}

void MainComponent::setKeyboardNoteBindings()
{
    const int keys[] = {65, 87, 83, 69, 68, 70, 84, 71, 89, 72, 85, 74, 75, 79, 76, 80, 59, 39, 93, 13};
    for (int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++)
    {
         keyboardNoteBindings.set(keys[i],i);
    }
}