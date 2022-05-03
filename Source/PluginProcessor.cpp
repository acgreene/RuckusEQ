/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
RuckusEQAudioProcessor::RuckusEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

RuckusEQAudioProcessor::~RuckusEQAudioProcessor()
{
}

//==============================================================================
const juce::String RuckusEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RuckusEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool RuckusEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool RuckusEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double RuckusEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RuckusEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int RuckusEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RuckusEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String RuckusEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void RuckusEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
// performs pre-playback initialization.
void RuckusEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // ProcessSpec struct is passed into a dsp algorithms prepare() method
    juce::dsp::ProcessSpec spec;
    
    // fill the ProcessSpec object with the audio block information.
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    auto chainSettings = getChainSettings(apvts);
    
    auto bandPassCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.bandPassFreq, chainSettings.bandPassQuality, juce::Decibels::decibelsToGain(chainSettings.bandPassGainInDecibels));
    
    *leftChain.get<ChainPositions::bandPass>().coefficients = *bandPassCoefficients;
    *rightChain.get<ChainPositions::bandPass>().coefficients = *bandPassCoefficients;
    
    //slope choice 0: 12 dB/Oct -> order: 2
    //slope choice 1: 24 dB/Oct -> order: 4
    //slope choice 2: 36 dB/Oct -> order: 6, etc.. => 2*(slope+1)=order
    auto filterCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.highPassFreq, sampleRate, 2*(chainSettings.highPassSlope + 1));
    
    auto& leftHighPass = leftChain.get<ChainPositions::highPass>();
    
    leftHighPass.setBypassed<0>(true);
    leftHighPass.setBypassed<1>(true);
    leftHighPass.setBypassed<2>(true);
    leftHighPass.setBypassed<3>(true);
    
    switch(chainSettings.highPassSlope)
    {
        case Slope_12:
        {
            *leftHighPass.get<0>().coefficients = *filterCoefficients[0];
            leftHighPass.setBypassed<0>(false);
            break;
        }
        case Slope_24:
        {
            *leftHighPass.get<0>().coefficients = *filterCoefficients[0];
            leftHighPass.setBypassed<0>(false);
            *leftHighPass.get<1>().coefficients = *filterCoefficients[1];
            leftHighPass.setBypassed<1>(false);
            break;
        }
        case Slope_36:
        {
            *leftHighPass.get<0>().coefficients = *filterCoefficients[0];
            leftHighPass.setBypassed<0>(false);
            *leftHighPass.get<1>().coefficients = *filterCoefficients[1];
            leftHighPass.setBypassed<1>(false);
            *leftHighPass.get<2>().coefficients = *filterCoefficients[2];
            leftHighPass.setBypassed<2>(false);
            break;
        }
        case Slope_48:
        {
            *leftHighPass.get<0>().coefficients = *filterCoefficients[0];
            leftHighPass.setBypassed<0>(false);
            *leftHighPass.get<1>().coefficients = *filterCoefficients[1];
            leftHighPass.setBypassed<1>(false);
            *leftHighPass.get<2>().coefficients = *filterCoefficients[2];
            leftHighPass.setBypassed<2>(false);
            *leftHighPass.get<3>().coefficients = *filterCoefficients[3];
            leftHighPass.setBypassed<3>(false);
            break;
        }
    }
    
    //right chain
    auto& rightHighPass = rightChain.get<ChainPositions::highPass>();
    
    rightHighPass.setBypassed<0>(true);
    rightHighPass.setBypassed<1>(true);
    rightHighPass.setBypassed<2>(true);
    rightHighPass.setBypassed<3>(true);
    
    switch(chainSettings.highPassSlope)
    {
        case Slope_12:
        {
            *rightHighPass.get<0>().coefficients = *filterCoefficients[0];
            rightHighPass.setBypassed<0>(false);
            break;
        }
        case Slope_24:
        {
            *rightHighPass.get<0>().coefficients = *filterCoefficients[0];
            rightHighPass.setBypassed<0>(false);
            *rightHighPass.get<1>().coefficients = *filterCoefficients[1];
            rightHighPass.setBypassed<1>(false);
            break;
        }
        case Slope_36:
        {
            *rightHighPass.get<0>().coefficients = *filterCoefficients[0];
            rightHighPass.setBypassed<0>(false);
            *rightHighPass.get<1>().coefficients = *filterCoefficients[1];
            rightHighPass.setBypassed<1>(false);
            *rightHighPass.get<2>().coefficients = *filterCoefficients[2];
            rightHighPass.setBypassed<2>(false);
            break;
        }
        case Slope_48:
        {
            *rightHighPass.get<0>().coefficients = *filterCoefficients[0];
            rightHighPass.setBypassed<0>(false);
            *rightHighPass.get<1>().coefficients = *filterCoefficients[1];
            rightHighPass.setBypassed<1>(false);
            *rightHighPass.get<2>().coefficients = *filterCoefficients[2];
            rightHighPass.setBypassed<2>(false);
            *rightHighPass.get<3>().coefficients = *filterCoefficients[3];
            rightHighPass.setBypassed<3>(false);
            break;
        }
    }
    
}

void RuckusEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool RuckusEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

//takes audio from the plugins input source, and feeds it through the plugins dsp.
void RuckusEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // clears any output channels that didn't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    auto chainSettings = getChainSettings(apvts);
    
    auto bandPassCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.bandPassFreq, chainSettings.bandPassQuality, juce::Decibels::decibelsToGain(chainSettings.bandPassGainInDecibels));
    
    *leftChain.get<ChainPositions::bandPass>().coefficients = *bandPassCoefficients;
    *rightChain.get<ChainPositions::bandPass>().coefficients = *bandPassCoefficients;

    //slope choice 0: 12 dB/Oct -> order: 2
    //slope choice 1: 24 dB/Oct -> order: 4
    //slope choice 2: 36 dB/Oct -> order: 6, etc.. => 2*(slope+1)=order
    auto filterCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.highPassFreq, getSampleRate(), 2*(chainSettings.highPassSlope + 1));
    
    auto& leftHighPass = leftChain.get<ChainPositions::highPass>();
    
    leftHighPass.setBypassed<0>(true);
    leftHighPass.setBypassed<1>(true);
    leftHighPass.setBypassed<2>(true);
    leftHighPass.setBypassed<3>(true);
    
    switch(chainSettings.highPassSlope)
    {
        case Slope_12:
        {
            *leftHighPass.get<0>().coefficients = *filterCoefficients[0];
            leftHighPass.setBypassed<0>(false);
            break;
        }
        case Slope_24:
        {
            *leftHighPass.get<0>().coefficients = *filterCoefficients[0];
            leftHighPass.setBypassed<0>(false);
            *leftHighPass.get<1>().coefficients = *filterCoefficients[1];
            leftHighPass.setBypassed<1>(false);
            break;
        }
        case Slope_36:
        {
            *leftHighPass.get<0>().coefficients = *filterCoefficients[0];
            leftHighPass.setBypassed<0>(false);
            *leftHighPass.get<1>().coefficients = *filterCoefficients[1];
            leftHighPass.setBypassed<1>(false);
            *leftHighPass.get<2>().coefficients = *filterCoefficients[2];
            leftHighPass.setBypassed<2>(false);
            break;
        }
        case Slope_48:
        {
            *leftHighPass.get<0>().coefficients = *filterCoefficients[0];
            leftHighPass.setBypassed<0>(false);
            *leftHighPass.get<1>().coefficients = *filterCoefficients[1];
            leftHighPass.setBypassed<1>(false);
            *leftHighPass.get<2>().coefficients = *filterCoefficients[2];
            leftHighPass.setBypassed<2>(false);
            *leftHighPass.get<3>().coefficients = *filterCoefficients[3];
            leftHighPass.setBypassed<3>(false);
            break;
        }
    }
    
    //right chain
    auto& rightHighPass = rightChain.get<ChainPositions::highPass>();
    
    rightHighPass.setBypassed<0>(true);
    rightHighPass.setBypassed<1>(true);
    rightHighPass.setBypassed<2>(true);
    rightHighPass.setBypassed<3>(true);
    
    switch(chainSettings.highPassSlope)
    {
        case Slope_12:
        {
            *rightHighPass.get<0>().coefficients = *filterCoefficients[0];
            rightHighPass.setBypassed<0>(false);
            break;
        }
        case Slope_24:
        {
            *rightHighPass.get<0>().coefficients = *filterCoefficients[0];
            rightHighPass.setBypassed<0>(false);
            *rightHighPass.get<1>().coefficients = *filterCoefficients[1];
            rightHighPass.setBypassed<1>(false);
            break;
        }
        case Slope_36:
        {
            *rightHighPass.get<0>().coefficients = *filterCoefficients[0];
            rightHighPass.setBypassed<0>(false);
            *rightHighPass.get<1>().coefficients = *filterCoefficients[1];
            rightHighPass.setBypassed<1>(false);
            *rightHighPass.get<2>().coefficients = *filterCoefficients[2];
            rightHighPass.setBypassed<2>(false);
            break;
        }
        case Slope_48:
        {
            *rightHighPass.get<0>().coefficients = *filterCoefficients[0];
            rightHighPass.setBypassed<0>(false);
            *rightHighPass.get<1>().coefficients = *filterCoefficients[1];
            rightHighPass.setBypassed<1>(false);
            *rightHighPass.get<2>().coefficients = *filterCoefficients[2];
            rightHighPass.setBypassed<2>(false);
            *rightHighPass.get<3>().coefficients = *filterCoefficients[3];
            rightHighPass.setBypassed<3>(false);
            break;
        }
    }
    
    // points to data in the audio buffer
    juce::dsp::AudioBlock<float> block(buffer);
    
    // extract individual left and right channels from buffer
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    
    // processing contexts that wrap each block
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    
    // pass contexts into mono filter chains
    leftChain.process(leftContext);
    rightChain.process(rightContext);
}

//==============================================================================
bool RuckusEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* RuckusEQAudioProcessor::createEditor()
{
    //return new RuckusEQAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void RuckusEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void RuckusEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    
    settings.highPassFreq = apvts.getRawParameterValue("HighPass Freq")->load();
    settings.highPassSlope = static_cast<Slope>(apvts.getRawParameterValue("HighPass Slope")->load());
    
    settings.lowPassFreq = apvts.getRawParameterValue("LowPass Freq")->load();
    settings.lowPassSlope = static_cast<Slope>(apvts.getRawParameterValue("LowPass Slope")->load());
    
    settings.bandPassFreq = apvts.getRawParameterValue("BandPass Freq")->load();
    settings.bandPassGainInDecibels = apvts.getRawParameterValue("BandPass Gain")->load();
    settings.bandPassQuality = apvts.getRawParameterValue("BandPass Q")->load();
    
    return settings;
}

//sets up all of the configurable parameters in the plugin to be passed into the audio processor value tree state constructor.
juce::AudioProcessorValueTreeState::ParameterLayout
    RuckusEQAudioProcessor::createParameterLayout()
{
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
       
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("HighPass Freq", 1),
                                                               "HighPass Freq",
                                                               juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                               20.f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("LowPass Freq", 1),
                                                               "LowPass Freq",
                                                               juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                               20000.f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("BandPass Freq", 1),
                                                               "BandPass Freq",
                                                               juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                               750.f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("BandPass Gain", 1),
                                                               "BandPass Gain",
                                                               juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                               0.0f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("BandPass Q", 1),
                                                               "BandPass Q",
                                                               juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                               1.f));
        
        juce::StringArray filterSlopes;
        for (int i = 0; i < 4; i++) {
            juce::String slope;
            slope << (12 + i*12);
            slope << " dB/Oct";
            filterSlopes.add(slope);
        }
        
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("HighPass Slope", 1), "HighPass Slope", filterSlopes, 0));
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("LowPass Slope", 1), "LowPass Slope", filterSlopes, 0));
        
        return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RuckusEQAudioProcessor();
}
