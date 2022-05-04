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
class RuckusEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    RuckusEQAudioProcessorEditor (RuckusEQAudioProcessor&);
    ~RuckusEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    RuckusEQAudioProcessor& audioProcessor;
    
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
    
    //function that will put all the sliders in a vector so we can iterate through them easily and apply processing on them as a batch if needed.
    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RuckusEQAudioProcessorEditor)
};
