/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
        
    }
};

struct CustomHorizontalSlider : juce::Slider
{
    CustomHorizontalSlider() : juce::Slider(juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
        
    }
};

struct CustomHorizontalBar : juce::Slider
{
    CustomHorizontalBar() : juce::Slider(juce::Slider::SliderStyle::LinearBar, juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
        
    }
};

//==============================================================================
/**
*/
// for the editor to respond to parameter changes, register it as a listener to all of them. therefore the editor will inherit from the class "Listener".
class RuckusEQAudioProcessorEditor  : public juce::AudioProcessorEditor,
juce::AudioProcessorParameter::Listener,
juce::Timer
{
public:
    RuckusEQAudioProcessorEditor (RuckusEQAudioProcessor&);
    ~RuckusEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    //editor callbacks happen on the audio thread, so we can't do any gui stuff (repainting, etc.) in the callback. instead we can set an atomic flag that the timer can check and update based on the flag.
    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {}
    void timerCallback() override; //query an atomic flag to decide if the chain needs updating and the component needs repainting.

private:
    RuckusEQAudioProcessor& audioProcessor;
    
    juce::Atomic<bool> parametersChanged {false};
    
    CustomRotarySlider  highPassFreqSlider,
                        rumbleFreqSlider, rumbleGainSlider,
                        lowFreqSlider, lowGainSlider,
                        lowMidFreqSlider, lowMidGainSlider,
                        highMidFreqSlider, highMidGainSlider,
                        highFreqSlider, highGainSlider,
                        airFreqSlider, airGainSlider,
                        lowPassFreqSlider;
    
    CustomHorizontalBar  rumbleQualitySlider,
                        lowQualitySlider,
                        lowMidQualitySlider,
                        highMidQualitySlider,
                        highQualitySlider,
                        airQualitySlider;
    
    CustomHorizontalSlider highPassSlopeSlider, lowPassSlopeSlider;
    
    //connect sliders to dsp parameters
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    
    Attachment  highPassFreqSliderAttachment,
                rumbleFreqSliderAttachment, rumbleGainSliderAttachment,
                lowFreqSliderAttachment, lowGainSliderAttachment,
                lowMidFreqSliderAttachment, lowMidGainSliderAttachment,
                highMidFreqSliderAttachment, highMidGainSliderAttachment,
                highFreqSliderAttachment, highGainSliderAttachment,
                airFreqSliderAttachment, airGainSliderAttachment,
                lowPassFreqSliderAttachment, rumbleQualitySliderAttachment,
                lowQualitySliderAttachment, lowMidQualitySliderAttachment,
                highMidQualitySliderAttachment, highQualitySliderAttachment,
                airQualitySliderAttachment, highPassSlopeSliderAttachment,
                lowPassSlopeSliderAttachment;
    
    
    //function that will put all the sliders in a vector so we can iterate through them easily and apply processing on them as a batch if needed.
    std::vector<juce::Component*> getComps();
    
    //add an instance of the monochain from plugin processor so we can use it's data to draw the response curve.
    MonoChain monoChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RuckusEQAudioProcessorEditor)
};
