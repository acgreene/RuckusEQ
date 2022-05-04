/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

ResponseCurveComponent::ResponseCurveComponent(RuckusEQAudioProcessor& p) : audioProcessor(p)
{
    //listen for when parameters change, grab parameters from audio processor and add ourselves as a listener to them.
    const auto& params = audioProcessor.getParameters();
    for(auto param : params)
    {
        param->addListener(this);
    }
    
    //start timer to check every 60Hz to see if we need to repaint the response curve
    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    //deregister as a listener when the destructor is called
    const auto& params = audioProcessor.getParameters();
    for(auto param : params)
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    //set atomic flag to true if a plugin parameter is changed
    parametersChanged.set(true);
}

//check if the parameters have been changed in the timer callback
void ResponseCurveComponent::timerCallback()
{
    //only refresh the curve if a change has been made- if a change has been made set the parametersChanged back to false so the curve isn't being continuously refreshed.
    if (parametersChanged.compareAndSetBool(false, true))
    {
        //update monochain
        auto chainSettings = getChainSettings(audioProcessor.apvts);
        
        auto rumbleCoefficients = makeRumbleFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::rumble>().coefficients, rumbleCoefficients);
        
        auto lowCoefficients = makeLowFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::low>().coefficients, lowCoefficients);
        
        auto lowMidCoefficients = makeLowMidFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::lowMid>().coefficients, lowMidCoefficients);
        
        auto highMidCoefficients = makeHighMidFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::highMid>().coefficients, highMidCoefficients);
        
        auto highCoefficients = makeHighFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::high>().coefficients, highCoefficients);
        
        auto airCoefficients = makeAirFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::air>().coefficients, airCoefficients);
        
        auto highPassCoefficients = makeHighPassFilter(chainSettings, audioProcessor.getSampleRate());
        updatePassFilter(monoChain.get<ChainPositions::highPass>(), highPassCoefficients, chainSettings.highPassSlope);
        
        auto lowPassCoefficients = makeLowPassFilter(chainSettings, audioProcessor.getSampleRate());
        updatePassFilter(monoChain.get<ChainPositions::lowPass>(), lowPassCoefficients, chainSettings.lowPassSlope);
        
        //signal a repaint so a new response curve is drawn
        repaint();
    }
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    
    //get response curve area boundaries
    auto responseArea = getLocalBounds();
    
    auto w = responseArea.getWidth();
    
    //get the parameters of each of the filters
    auto& highPass = monoChain.get<ChainPositions::highPass>();
    auto& rumble = monoChain.get<ChainPositions::rumble>();
    auto& low = monoChain.get<ChainPositions::low>();
    auto& lowMid = monoChain.get<ChainPositions::lowMid>();
    auto& highMid = monoChain.get<ChainPositions::highMid>();
    auto& high = monoChain.get<ChainPositions::high>();
    auto& air = monoChain.get<ChainPositions::air>();
    auto& lowPass = monoChain.get<ChainPositions::lowPass>();
    
    auto sampleRate = audioProcessor.getSampleRate();
    
    //create a vector to store the magnitudes of each filter
    std::vector<double> mags;
    
    //compute one magnitude per pixel, length of entire magnitude vector is equal to pixel length of the width of the response area.
    mags.resize(w);
    
    //iterate through each pixel and compute the magnitude at that frequency
    for (int i = 0; i < w; i++)
    {
        //starting gain of 1
        double mag = 1.f;
        
        //helper function to map magnitudes from pixel space to frequency space
        auto freq = mapToLog10(double(i) / double (w), 20.0, 22000.0);
        
        //call magnitude function with freq and multiply the results by mag, make sure band isn't bypassed.
        if(!monoChain.isBypassed<ChainPositions::rumble>())
            mag *= rumble.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        if(!monoChain.isBypassed<ChainPositions::low>())
            mag *= low.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        if(!monoChain.isBypassed<ChainPositions::lowMid>())
            mag *= lowMid.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        if(!monoChain.isBypassed<ChainPositions::highMid>())
            mag *= highMid.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        if(!monoChain.isBypassed<ChainPositions::high>())
            mag *= high.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        if(!monoChain.isBypassed<ChainPositions::air>())
            mag *= air.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        if(!highPass.isBypassed<0>())
            mag *= highPass.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!highPass.isBypassed<1>())
            mag *= highPass.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!highPass.isBypassed<2>())
            mag *= highPass.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!highPass.isBypassed<3>())
            mag *= highPass.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        if(!lowPass.isBypassed<0>())
            mag *= lowPass.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!lowPass.isBypassed<1>())
            mag *= lowPass.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!lowPass.isBypassed<2>())
            mag *= lowPass.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!lowPass.isBypassed<3>())
            mag *= lowPass.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        //convert gain to decibels
        mags[i] = Decibels::gainToDecibels(mag);
    }
    
    //create a path in order to convert vector of magnitudes into a path
    Path responseCurve;
    
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };
    
    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    
    for(size_t i = 1; i < mags.size(); i++)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }
    
    g.setColour(Colours::orange);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
    
    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

//==============================================================================
RuckusEQAudioProcessorEditor::RuckusEQAudioProcessorEditor (RuckusEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
responseCurveComponent(audioProcessor),
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
    using namespace juce;
    
    //color background of plugin
    g.fillAll (Colours::black);
}

//here is where the positions of the gui components are layed out in the plugin window
void RuckusEQAudioProcessorEditor::resized()
{
    //boundary of the entire plugin window
    auto bounds = getLocalBounds();
    
    //allocate top 40% of the plugin window for the frequency response curve
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.6);
    responseCurveComponent.setBounds(responseArea);
    
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
        &lowPassFreqSlider, &highPassSlopeSlider, &lowPassSlopeSlider,
        &responseCurveComponent
    };
}
