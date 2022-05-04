/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
RuckusEQAudioProcessorEditor::RuckusEQAudioProcessorEditor (RuckusEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
highPassFreqSliderAttachment(audioProcessor.apvts, "HighPass Freq", highPassFreqSlider),
rumbleFreqSliderAttachment(audioProcessor.apvts, "Rumble Freq", rumbleFreqSlider),
rumbleGainSliderAttachment(audioProcessor.apvts, "Rumble Gain", rumbleGainSlider),
lowFreqSliderAttachment(audioProcessor.apvts, "Low Freq", lowFreqSlider),
lowGainSliderAttachment(audioProcessor.apvts, "Low Gain", lowGainSlider),
lowMidFreqSliderAttachment(audioProcessor.apvts, "LowMid Freq", lowMidFreqSlider),
lowMidGainSliderAttachment(audioProcessor.apvts, "LowMid Gain", lowMidGainSlider),
highMidFreqSliderAttachment(audioProcessor.apvts, "HighMid Freq", highMidFreqSlider),
highMidGainSliderAttachment(audioProcessor.apvts, "HighMid Gain", highMidGainSlider),
highFreqSliderAttachment(audioProcessor.apvts, "High Freq", highFreqSlider),
highGainSliderAttachment(audioProcessor.apvts, "High Gain", highGainSlider),
airFreqSliderAttachment(audioProcessor.apvts, "Air Freq", airFreqSlider),
airGainSliderAttachment(audioProcessor.apvts, "Air Gain", airGainSlider),
lowPassFreqSliderAttachment(audioProcessor.apvts, "LowPass Freq", lowPassFreqSlider),
rumbleQualitySliderAttachment(audioProcessor.apvts, "Rumble Q", rumbleQualitySlider),
lowQualitySliderAttachment(audioProcessor.apvts, "Low Q", lowQualitySlider),
lowMidQualitySliderAttachment(audioProcessor.apvts, "LowMid Q", lowMidQualitySlider),
highMidQualitySliderAttachment(audioProcessor.apvts, "HighMid Q", highMidQualitySlider),
highQualitySliderAttachment(audioProcessor.apvts, "High Q", highQualitySlider),
airQualitySliderAttachment(audioProcessor.apvts, "Air Q", airQualitySlider),
highPassSlopeSliderAttachment(audioProcessor.apvts, "HighPass Slope", highPassSlopeSlider),
lowPassSlopeSliderAttachment(audioProcessor.apvts, "LowPass Slope", lowPassSlopeSlider)
{
    //batch add all of the sliders to the gui
    for(auto* comp: getComps())
    {
        addAndMakeVisible(comp);
    }
    setSize (800, 533);
}

RuckusEQAudioProcessorEditor::~RuckusEQAudioProcessorEditor()
{
}

//==============================================================================
void RuckusEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

//here is where the positions of the gui components are layed out in the plugin window
void RuckusEQAudioProcessorEditor::resized()
{
    //boundary of the entire plugin window
    auto bounds = getLocalBounds();
    
    //allocate top 40% of the plugin window for the frequency response curve
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.6);
    
    //allocate left 12.5% of bottom of window for HPF
    auto highPassArea = bounds.removeFromLeft(bounds.getWidth() * 0.125);
    highPassFreqSlider.setBounds(highPassArea.removeFromTop(highPassArea.getHeight() * 0.7));
    highPassSlopeSlider.setBounds(highPassArea);
    
    auto rumbleArea = bounds.removeFromLeft(bounds.getWidth() * (12.5/87.5));
    rumbleFreqSlider.setBounds(rumbleArea.removeFromTop(rumbleArea.getHeight() * 0.4));
    rumbleGainSlider.setBounds(rumbleArea.removeFromTop(rumbleArea.getHeight() * 0.667));
    rumbleQualitySlider.setBounds(rumbleArea);
    
    auto lowArea = bounds.removeFromLeft(bounds.getWidth() * (12.5/75));
    lowFreqSlider.setBounds(lowArea.removeFromTop(lowArea.getHeight() * 0.4));
    lowGainSlider.setBounds(lowArea.removeFromTop(lowArea.getHeight() * 0.667));
    lowQualitySlider.setBounds(lowArea);

    auto lowMidArea = bounds.removeFromLeft(bounds.getWidth() * (12.5/62.5));
    lowMidFreqSlider.setBounds(lowMidArea.removeFromTop(lowMidArea.getHeight() * 0.4));
    lowMidGainSlider.setBounds(lowMidArea.removeFromTop(lowMidArea.getHeight() * 0.667));
    lowMidQualitySlider.setBounds(lowMidArea);

    auto highMidArea = bounds.removeFromLeft(bounds.getWidth() * (12.5/50));
    highMidFreqSlider.setBounds(highMidArea.removeFromTop(highMidArea.getHeight() * 0.4));
    highMidGainSlider.setBounds(highMidArea.removeFromTop(highMidArea.getHeight() * 0.667));
    highMidQualitySlider.setBounds(highMidArea);

    auto highArea = bounds.removeFromLeft(bounds.getWidth() * (12.5/37.5));
    highFreqSlider.setBounds(highArea.removeFromTop(highArea.getHeight() * 0.4));
    highGainSlider.setBounds(highArea.removeFromTop(highArea.getHeight() * 0.667));
    highQualitySlider.setBounds(highArea);

    auto airArea = bounds.removeFromLeft(bounds.getWidth() * (12.5/25));
    airFreqSlider.setBounds(airArea.removeFromTop(airArea.getHeight() * 0.4));
    airGainSlider.setBounds(airArea.removeFromTop(airArea.getHeight() * 0.667));
    airQualitySlider.setBounds(airArea);
    
    auto lowPassArea = bounds.removeFromRight(bounds.getWidth() * 1);
    lowPassFreqSlider.setBounds(lowPassArea.removeFromTop(lowPassArea.getHeight() * 0.7));
    lowPassSlopeSlider.setBounds(lowPassArea);

}

std::vector<juce::Component*> RuckusEQAudioProcessorEditor::getComps()
{
    return
    {
        &highPassFreqSlider,
        &rumbleFreqSlider, &rumbleGainSlider, &rumbleQualitySlider,
        &lowFreqSlider, &lowGainSlider, &lowQualitySlider,
        &lowMidFreqSlider, &lowMidGainSlider, &lowMidQualitySlider,
        &highMidFreqSlider, &highMidGainSlider, &highMidQualitySlider,
        &highFreqSlider, &highGainSlider, &highQualitySlider,
        &airFreqSlider, &airGainSlider, &airQualitySlider,
        &lowPassFreqSlider, &highPassSlopeSlider, &lowPassSlopeSlider
    };
}
