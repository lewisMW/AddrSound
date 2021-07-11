#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class EffectSettings : public juce::Component
{
public:
    EffectSettings()
      : gainControls(*this, 1.0, 1.0, "Gain", juce::Range<double>(0.9,1.1),true),
        vibratoControls(*this, 0.0, 0.2, "Vibrato", juce::Range<double>(0.0,1.0),false),
        distortionControls(*this, 0.0, 1.0, "Distortion", juce::Range<double>(0.0,1.0),false),
        reverbControls(*this, 0.0, 1.0,  "Reverb", juce::Range<double>(0.0,1.0),false),
        midiControls(*this, (double)PlaybackControlState::Play, 1.0, "MIDI"),

        controlArray({&gainControls, &vibratoControls, &distortionControls, &reverbControls, &midiControls})
    {}

    enum ControlID {Gain=0, Vibrato, Distortion, Reverb, Midi};
    enum PlaybackControlState {Play = 0, Pause = 1, Stop = 2};

    std::atomic<double>* getControlValue(ControlID id)
    {
        return controlArray[id]->getParameter();
    }

    void showControl(ControlID id)
    {
        hideAll();
        controlArray[id]->setShowing(true);
    }

    void resized() override
    {
        for (Control* c : controlArray) c->setBounds();
    }

    void addCustomCallback(ControlID id, std::function<void (std::atomic<double>&)> callback)
    {
        controlArray[id]->addCustomCallback(callback);
    }


private:

    struct Control
    {
        Control(EffectSettings& container, double defaultValue, double scaler)
            : container(container), parameter(defaultValue*scaler), scaler(scaler),
              defaultValue(defaultValue), customCallback(nullptr) {}

        std::atomic<double>* getParameter()
        {
            return &parameter;
        }

        virtual void setShowing(bool show)=0;
        virtual void setBounds()=0;

        void addCustomCallback(std::function<void (std::atomic<double>&)>  callback)
        {
            customCallback = callback;
        }
        EffectSettings& container;
        double scaler;
        double defaultValue;
        std::atomic<double> parameter;
        std::function<void (std::atomic<double>&)>  customCallback;
    };

    struct SliderControl : Control
    {
        SliderControl(EffectSettings& container, double defaultValue, double scaler, juce::String labelName, juce::Range<double> valueRange, bool joyStick)
            : Control(container, defaultValue, scaler),
              slider(juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxRight),
              label(labelName+"Label", labelName),  joyStick(joyStick)
        {
            container.addChildComponent(slider);
            container.addChildComponent(label);

            slider.setRange(valueRange, 0.01);
            slider.setValue(defaultValue);

            label.attachToComponent(&slider, true);

            slider.onValueChange = [this] {
                this->parameter = this->scaler*slider.getValue();
                if (this->customCallback != nullptr) this->customCallback(parameter);
            };
            
            if (this->joyStick) slider.onDragEnd = [this] {
                slider.setValue(this->defaultValue);
            };
        }

        void setShowing(bool show)
        {
            slider.setVisible(show);
            label.setVisible(show);
            slider.setEnabled(show);
            label.setEnabled(show);
        }
        void setBounds()
        {
            slider.setBoundsRelative(0.2f, 0.1f, 0.75f, 0.8f);
        }

        juce::Slider slider;
        juce::Label label;
        bool joyStick;

    } gainControls, vibratoControls, distortionControls, reverbControls;

    struct PlaybackControl : Control
    {
        PlaybackControl(EffectSettings& container, double defaultValue, double scaler, juce::String labelName)
            : Control(container, defaultValue, scaler),
              pauseButton("Pause"), stopButton("Stop"),
              label(labelName+"Label", labelName)
        {
            container.addChildComponent(label);
            container.addChildComponent(pauseButton);
            container.addChildComponent(stopButton);

            label.attachToComponent(&pauseButton, true);

            pauseButton.onClick = [this] {
                if (this->parameter == (double) PlaybackControlState::Play)
                {
                    this->parameter = (double) PlaybackControlState::Pause;
                    pauseButton.setButtonText("Play");
                }
                else
                {
                    this->parameter = (double) PlaybackControlState::Play;
                    pauseButton.setButtonText("Pause");
                }
                if (this->customCallback != nullptr) this->customCallback(parameter);
            };
            stopButton.onClick = [this] {
                this->parameter = (double) PlaybackControlState::Stop;
                if (this->customCallback != nullptr) this->customCallback(parameter);
            };

        }

        void setShowing(bool show)
        {
            pauseButton.setVisible(show);
            stopButton.setVisible(show);
            label.setVisible(show);
            pauseButton.setEnabled(show);
            stopButton.setEnabled(show);
            label.setEnabled(show);
        }
        void setBounds()
        {
            pauseButton.setBoundsRelative(0.2f, 0.1f, 0.35f, 0.8f);
            stopButton.setBoundsRelative(0.6f, 0.1f, 0.35f, 0.8f);
        }
        juce::Label label;
        juce::TextButton pauseButton;
        juce::TextButton stopButton;
    } midiControls;


    juce::Array<Control*> controlArray;

    void hideAll()
    {
        for (Control* c : controlArray) c->setShowing(false);
    }

};