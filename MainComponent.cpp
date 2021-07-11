#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : additiveSpectrum(30, 440, 0.5f, 10),
      refSpectrum(512, 512, 2),
      spectrumEditor(additiveSpectrum, refSpectrum),
      timeSlider(additiveSpectrum, spectrumEditor),
      midiPlayer(additiveSpectrum, timeSlider, effectSettings.getControlValue(EffectSettings::ControlID::Midi))
{
    level = 0.0f;

    createWaveTable();

    setSize (500, 450);
    setAudioChannels(0,2); // Only outputs

    setWantsKeyboardFocus(true);
    addKeyListener(this);

    addAndMakeVisible(spectrumEditor);
    addMouseListener(&spectrumEditor, false);
    addAndMakeVisible(timeSlider);
    addMouseListener(&timeSlider, false);

    addAndMakeVisible(toolsButton);
    toolsButton.onChange = [this] {toolsMenuSelect();};

    circularPointerBufferIndex = 0;
    addAndMakeVisible(refAudioPositionSlider);
    refAudioPositionSlider.setTextValueSuffix(" s");
    refAudioPositionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
    refAudioPositionSlider.setEnabled(false);
    addMouseListener(&refAudioPositionSlider, false);

    addAndMakeVisible(effectSettings);
    effectSettings.addCustomCallback(EffectSettings::ControlID::Gain, [this] (std::atomic<double>& delta) {
        spectrumEditor.multiplyAllPoints(delta);
        spectrumEditor.repaint();
    });

    setKeyboardNoteBindings();    
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
    squareTable.setSize(1, (int) tableSize + 1);
    auto* squareSamples = squareTable.getWritePointer(0);
    auto angleDelta = juce::MathConstants<double>::twoPi / (double) (tableSize-1);
    auto currentAngle = 0.0;

    for (unsigned int i=0; i < tableSize; i++)
    {
        auto sample = std::sin(currentAngle);
        samples[i] = (float) sample;
        squareSamples[i] = (float) std::tanh(50*sample); // mimick a square
        currentAngle += angleDelta;
    }
    samples[tableSize] = samples[0];
    squareSamples[tableSize] = samples[0];
}

void MainComponent::prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate)
{
    for(auto i=0; i < additiveSpectrum.getNFreqs(); i++)
    {
        auto* oscillator = new WavetableOscillator(sineTable, squareTable, (float) sampleRate);
        oscillator->setAmplitude(additiveSpectrum.getMagnitude(i));
        oscillator->setFrequency(additiveSpectrum.getFrequency(i));
        oscillator->setVibratoFactor(effectSettings.getControlValue(EffectSettings::ControlID::Vibrato));
        oscillator->setDistortionFactor(effectSettings.getControlValue(EffectSettings::ControlID::Distortion));
        oscillators.add(oscillator);
    }
    level = 0.5f / (float) additiveSpectrum.getNFreqs();

    reverb.setSampleRate(sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* leftBuffer = bufferToFill.buffer->getWritePointer(0,bufferToFill.startSample);
    auto* rightBuffer = bufferToFill.buffer->getWritePointer(1,bufferToFill.startSample);
    bufferToFill.clearActiveBufferRegion();

    if (refPlaying.load(std::memory_order_acquire) == true) // Play reference audio
    {
        std::shared_ptr<Spectrum::Peaks> peaks = std::atomic_load_explicit(&fftPeaks, std::memory_order_acquire);
        auto nOscillators = std::min<int>(oscillators.size(), (int) peaks->indexs.size());
        for(auto oIndex=0; oIndex < nOscillators; oIndex++)
        {
            auto peakIndex = peaks->indexs[oIndex];
            auto peakValue = peaks->values[oIndex];
            auto* oscillator = oscillators.getUnchecked(oIndex);
            oscillator->setAmplitude(peakValue);
            oscillator->setFrequency(refSpectrum.getFrequency(peakIndex));

            for (auto sample = 0; sample < bufferToFill.numSamples; sample++)
            {
                auto current = 0.0f;
                current += oscillator->getNextSample() * level;
                leftBuffer[sample] += current;
                rightBuffer[sample] += current;
            }
        }
    }
    else // Play additive composition (Fourier Series)
    {
        auto nOscillators = oscillators.size();
        for(auto oIndex=0; oIndex < nOscillators; oIndex++)
        {
            auto* oscillator = oscillators.getUnchecked(oIndex);
            oscillator->setAmplitude(additiveSpectrum.getMagnitude(oIndex));
            oscillator->setFrequency(additiveSpectrum.getFrequency(oIndex));

            for (auto sample = 0; sample < bufferToFill.numSamples; sample++)
            {
                auto current = 0.0f; 
                // Playing additive oscillators
                
                current += oscillator->getNextSample() * level;
                leftBuffer[sample] += current;
                rightBuffer[sample] += current;
            }
        }
        // Process reverb:
        std::atomic<double>* reverbFactor = effectSettings.getControlValue(EffectSettings::ControlID::Reverb);
        if (reverbFactor)
        {
            reverbParameters.roomSize = (float) *reverbFactor;
            reverbParameters.wetLevel = (float) *reverbFactor;
            reverb.setParameters(reverbParameters);
            reverb.processStereo(leftBuffer, rightBuffer, bufferToFill.numSamples);
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
    timeSlider.setBounds(50,360,400,20);
    effectSettings.setBounds(50, 10, 300, 30);
    toolsButton.setBounds(350, 10, 100, 30);
    refAudioPositionSlider.setBounds(50, 400, 400, 40);
}


// Tools Menu:
MainComponent::ToolsButton::ToolsButton()
    : juce::ComboBox("Tools")
{
    setText("Tools");
    addItem(juce::String("Save Spectrum"), ItemIDs::SaveID);
    addItem(juce::String("Load Spectrum"), ItemIDs::LoadID);
    addItem(juce::String("Play MIDI File"), ItemIDs::MidiID);
    addItem(juce::String("Gain"), ItemIDs::GainID);
    addItem(juce::String("Vibrato"), ItemIDs::VibratoID);
    addItem(juce::String("Distortion"), ItemIDs::DistortionID);
    addItem(juce::String("Reverb"), ItemIDs::ReverbID);
    addItem(juce::String("Load Reference Spectrum"), ItemIDs::LoadRefID);
    
}
void MainComponent::toolsMenuSelect()
{
    
    ToolsButton::ItemIDs id = (ToolsButton::ItemIDs) toolsButton.getSelectedId();
    switch (id)
    {
        case ToolsButton::ItemIDs::SaveID:
            saveSpectrum();
            break;
        case ToolsButton::ItemIDs::LoadID:
            loadSpectrum();
            break;
        case ToolsButton::ItemIDs::MidiID:
            effectSettings.showControl(EffectSettings::ControlID::Midi);
            if (!midiPlayer.isTimerRunning()) loadMidi();
            break;
        case ToolsButton::ItemIDs::GainID:
            effectSettings.showControl(EffectSettings::ControlID::Gain);
            break;
        case ToolsButton::ItemIDs::VibratoID:
            effectSettings.showControl(EffectSettings::ControlID::Vibrato);
            break;
        case ToolsButton::ItemIDs::DistortionID:
            effectSettings.showControl(EffectSettings::ControlID::Distortion);
            break;
        case ToolsButton::ItemIDs::ReverbID:
            effectSettings.showControl(EffectSettings::ControlID::Reverb);
            break;
        case ToolsButton::ItemIDs::LoadRefID:   
            loadReferenceFile();
    }
    toolsButton.setText("Tools");
}
void MainComponent::saveSpectrum()
{
    juce::FileChooser fC("Save spectrum file as", juce::File(), "*.addrsound");
    if (fC.browseForFileToSave(true))
    {
        juce::File file = fC.getResult();
        juce::FileOutputStream fileStream(file);
        if (fileStream.openedOk())
        {
            fileStream.setPosition(0);
            fileStream.truncate();
            additiveSpectrum.saveSpectrum(fileStream);
            fileStream.flush();
        }
    }
}
void MainComponent::loadSpectrum()
{
    deviceManager.closeAudioDevice(); // Stop sound thread to allow for the change of time-critical data structures:
    juce::FileChooser fC("Load spectrum file", juce::File(), "*.addrsound");
    if (fC.browseForFileToOpen())
    {
        juce::File file = fC.getResult();
        juce::FileInputStream fileStream(file);
        if (fileStream.openedOk()) additiveSpectrum.loadSpectrum(fileStream);
    }
    deviceManager.restartLastAudioDevice(); // Start sound thread again
    spectrumEditor.initPoints();
    spectrumEditor.repaint();
    timeSlider.repaint();
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
            

            refAudioPositionSlider.setValue(0.0);
            refAudioPositionSlider.setRange(0.0, refSpectrum.getDuration(), refSpectrum.getWindowPeriodSeconds()/4);
            refAudioPositionSlider.onValueChange = [this] {
                refSpectrum.setTime((float) refAudioPositionSlider.getValue());                
                refSpectrum.refreshFFT();

                std::shared_ptr<Spectrum::Peaks> peaks(new Spectrum::Peaks());
                refSpectrum.calcPeaks(*peaks);
                std::atomic_store_explicit(&fftPeaks,peaks, std::memory_order_release);
                circularPointerBuffer[circularPointerBufferIndex++] = peaks;
                circularPointerBufferIndex %= circularPointerBuffer.size();
                refPlaying.store(true, std::memory_order_release);

                spectrumEditor.refreshPoints(true, peaks.get());
                spectrumEditor.repaint();
            };
            
            refAudioPositionSlider.onDragEnd = [this] {
                refPlaying.store(false, std::memory_order_release);
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

void MainComponent::loadMidi()
{
    midiPlayer.stopTimer(); // ensure player thread has stopped, as the following will delete current midi data

    juce::FileChooser fC("Load MIDI file");
    if (fC.browseForFileToOpen())
    {
        juce::File file = fC.getResult();
        juce::FileInputStream fileStream(file);
   
        if (fileStream.openedOk() && mFile.readFrom(fileStream) && mFile.getNumTracks())
        {
            int nTracks = mFile.getNumTracks();
            mFile.convertTimestampTicksToSeconds();
            int trackIndex = 0;
            if (nTracks > 1)
            {
                juce::AlertWindow chooseTrack("Choose the MIDI track to play", "The choosen MIDI file has "+juce::String(nTracks)+" tracks. Enter which one to play below:",juce::AlertWindow::AlertIconType::QuestionIcon);
                juce::StringArray trackNames;
                for (int i=0; i<nTracks; i++)
                {
                    juce::String trackName = "Untitled";
                    auto* track = mFile.getTrack(i);
                    if (track)
                    {
                        for (auto* midiEvent : *track)
                        {
                            auto msg = midiEvent->message;
                            if (msg.isTrackNameEvent())
                            {
                                trackName = msg.getTextFromTextMetaEvent();
                                break;
                            }
                        }
                    }
                    trackNames.add(trackName);
                }
                chooseTrack.addComboBox("trackSelection", trackNames, "Track Selection");
                chooseTrack.addButton("Select",0);
                chooseTrack.runModalLoop();
                trackIndex = chooseTrack.getComboBoxComponent("trackSelection")->getSelectedItemIndex();
            }
            auto* track = mFile.getTrack(trackIndex);
            midiPlayer.play(track);
        }
        else
        {
            juce::AlertWindow::showMessageBox(juce::AlertWindow::AlertIconType::WarningIcon,
            "Error Reading MIDI File",
                "The file selected (" + file.getFileName() + ") could not be read as a MIDI file or contained no tracks.");
        }
    }
}

MainComponent::MidiTimer::MidiTimer(AdditiveSpectrum& additiveSpectrum, TimeSlider& timeSlider, std::atomic<double>* midiControlState)
    : additiveSpectrum(additiveSpectrum), timeSlider(timeSlider), midiControlState(midiControlState) {}

void MainComponent::MidiTimer::play(const juce::MidiMessageSequence *track)
{
    eventIndex = 0;
    currentTime = 0.0;
    currentMsg = nullptr;
    nEvents = 0;
    if (track)
    {
        *midiControlState = (double) EffectSettings::PlaybackControlState::Play;
        midiTrack = track;
        nEvents = midiTrack->getNumEvents();
        scheduleNextNote();
    }
}

void MainComponent::MidiTimer::scheduleNextNote()
{
    for (; eventIndex < nEvents; eventIndex++)
    {
        auto* event = midiTrack->getEventPointer(eventIndex);
        auto* msg = &event->message;
        if (msg->isNoteOn())
        {
            double timeToWait = msg->getTimeStamp() - currentTime; // in seconds
            int timeToWaitMilliseconds = (int) (timeToWait*1000);
            if (timeToWaitMilliseconds == 0) continue; // Skip polyphony
            currentMsg = msg;
            eventIndex++; // Have to increment before timer blocks this UI thread (if first note)
            this->startTimer(timeToWaitMilliseconds);
            break;
        }
    }
    if (eventIndex == nEvents) this->stopTimer();
}

void MainComponent::MidiTimer::hiResTimerCallback()
{
    // Check if user has requested to pause/stop playback:
    if (*midiControlState != (double) EffectSettings::PlaybackControlState::Play)
    {
        if (*midiControlState == (double) EffectSettings::PlaybackControlState::Pause) this->startTimer(500);
        if (*midiControlState == (double) EffectSettings::PlaybackControlState::Stop) this->stopTimer();
        return;
    }
    // Play Midi note:
    if (currentMsg)
    {
        float freq = (float) juce::MidiMessage::getMidiNoteInHertz(currentMsg->getNoteNumber());
        additiveSpectrum.setFirstFrequency((float) freq);

        const juce::MessageManagerLock mmLock; // Lock UI thread whilst updating sound graphics:
        timeSlider.playSound();

        currentTime = currentMsg->getTimeStamp();
        scheduleNextNote();
    }
}

//==============================================================================
bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* /*originatingComponent*/)
{
    auto keyCode = key.getKeyCode();
    
    juce::ModifierKeys keyMods = key.getModifiers();
    if (keyMods.isCommandDown()) // (CTRL or command key) Copy and paste keyframes
    {
        if (keyCode == 67) // 'c' copy
        {
            additiveSpectrum.copyKeyFrame();
        }
        else if (keyCode == 86) // 'v' paste
        {
            additiveSpectrum.pasteKeyFrame();
            spectrumEditor.refreshPoints();
            spectrumEditor.repaint();
            timeSlider.repaint();
        }
    }
    else if (keyboardNoteBindings.contains(keyCode)) // Play musical note
    {
        auto noteOffset = keyboardNoteBindings[keyCode];
        auto midiNote = 69 + noteOffset;
        auto freq = 440.0*pow(2.0, (midiNote-69.0)/12.0);
        additiveSpectrum.setFirstFrequency((float) freq);
        timeSlider.playSound();
        return true;
    }
    return false;
}

void MainComponent::setKeyboardNoteBindings()
{
    const int keys[] = {65, 87, 83, 69, 68, 70, 84, 71, 89, 72, 85, 74, 75, 79, 76, 80, 59, 39, 93, 13};
    for (int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++)
    {
         keyboardNoteBindings.set(keys[i],i);
    }
}