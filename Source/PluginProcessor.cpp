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
    
    updateFilters();
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
    
    updateFilters();
    
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
    return new RuckusEQAudioProcessorEditor (*this);
}

//==============================================================================
//plug-in state is housed here. use this to recall plugin parameters
void RuckusEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    //memory output stream writes apvts state to memory block
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

//restore plugin state from memory, save plug-in parameters between loads.
void RuckusEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if(tree.isValid())
    {
        apvts.replaceState(tree);
        updateFilters();
    }
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    
    settings.highPassFreq = apvts.getRawParameterValue("HighPass Freq")->load();
    settings.highPassSlope = static_cast<Slope>(apvts.getRawParameterValue("HighPass Slope")->load());
    
    settings.lowPassFreq = apvts.getRawParameterValue("LowPass Freq")->load();
    settings.lowPassSlope = static_cast<Slope>(apvts.getRawParameterValue("LowPass Slope")->load());
    
    settings.rumbleFreq = apvts.getRawParameterValue("Rumble Freq")->load();
    settings.rumbleGainInDecibels = apvts.getRawParameterValue("Rumble Gain")->load();
    settings.rumbleQuality = apvts.getRawParameterValue("Rumble Q")->load();
    
    settings.lowFreq = apvts.getRawParameterValue("Low Freq")->load();
    settings.lowGainInDecibels = apvts.getRawParameterValue("Low Gain")->load();
    settings.lowQuality = apvts.getRawParameterValue("Low Q")->load();
    
    settings.lowMidFreq = apvts.getRawParameterValue("LowMid Freq")->load();
    settings.lowMidGainInDecibels = apvts.getRawParameterValue("LowMid Gain")->load();
    settings.lowMidQuality = apvts.getRawParameterValue("LowMid Q")->load();
    
    settings.highMidFreq = apvts.getRawParameterValue("HighMid Freq")->load();
    settings.highMidGainInDecibels = apvts.getRawParameterValue("HighMid Gain")->load();
    settings.highMidQuality = apvts.getRawParameterValue("HighMid Q")->load();
    
    settings.highFreq = apvts.getRawParameterValue("High Freq")->load();
    settings.highGainInDecibels = apvts.getRawParameterValue("High Gain")->load();
    settings.highQuality = apvts.getRawParameterValue("High Q")->load();
    
    settings.airFreq = apvts.getRawParameterValue("Air Freq")->load();
    settings.airGainInDecibels = apvts.getRawParameterValue("Air Gain")->load();
    settings.airQuality = apvts.getRawParameterValue("Air Q")->load();
    
    return settings;
}

void RuckusEQAudioProcessor::updateBandPassFilter(const ChainSettings & chainSettings)
{
    //rumble
    auto rumbleCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.rumbleFreq, chainSettings.rumbleQuality, juce::Decibels::decibelsToGain(chainSettings.rumbleGainInDecibels));
    
    updateCoefficients(leftChain.get<ChainPositions::rumble>().coefficients, rumbleCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::rumble>().coefficients, rumbleCoefficients);
    
    //lows
    auto lowCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.lowFreq, chainSettings.lowQuality, juce::Decibels::decibelsToGain(chainSettings.lowGainInDecibels));
    
    updateCoefficients(leftChain.get<ChainPositions::low>().coefficients, lowCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::low>().coefficients, lowCoefficients);
    
    //low-mids
    auto lowMidCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.lowMidFreq, chainSettings.lowMidQuality, juce::Decibels::decibelsToGain(chainSettings.lowMidGainInDecibels));
    
    updateCoefficients(leftChain.get<ChainPositions::lowMid>().coefficients, lowMidCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::lowMid>().coefficients, lowMidCoefficients);
    
    //high-mids
    auto highMidCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.highMidFreq, chainSettings.highMidQuality, juce::Decibels::decibelsToGain(chainSettings.highMidGainInDecibels));
    
    updateCoefficients(leftChain.get<ChainPositions::highMid>().coefficients, highMidCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::highMid>().coefficients, highMidCoefficients);
    
    //highs
    auto highCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.highFreq, chainSettings.highQuality, juce::Decibels::decibelsToGain(chainSettings.highGainInDecibels));
    
    updateCoefficients(leftChain.get<ChainPositions::high>().coefficients, highCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::high>().coefficients, highCoefficients);
    
    //air
    auto airCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.airFreq, chainSettings.airQuality, juce::Decibels::decibelsToGain(chainSettings.airGainInDecibels));
    
    updateCoefficients(leftChain.get<ChainPositions::air>().coefficients, airCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::air>().coefficients, airCoefficients);
}

void RuckusEQAudioProcessor::updateCoefficients(Coefficients& old, const Coefficients &replacements)
{
    *old = *replacements;
}

void RuckusEQAudioProcessor::updateHighPassFilters(const ChainSettings &chainSettings)
{
    auto highPassCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.highPassFreq, getSampleRate(), 2*(chainSettings.highPassSlope + 1));
    
    auto& leftHighPass = leftChain.get<ChainPositions::highPass>();
    updatePassFilter(leftHighPass, highPassCoefficients, chainSettings.highPassSlope);
    
    auto& rightHighPass = rightChain.get<ChainPositions::highPass>();
    updatePassFilter(rightHighPass, highPassCoefficients, chainSettings.highPassSlope);
}

void RuckusEQAudioProcessor::updateLowPassFilters(const ChainSettings &chainSettings)
{
    auto lowPassCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.lowPassFreq, getSampleRate(), 2*(chainSettings.lowPassSlope + 1));
    
    auto& leftLowPass = leftChain.get<ChainPositions::lowPass>();
    updatePassFilter(leftLowPass, lowPassCoefficients, chainSettings.lowPassSlope);

    auto& rightLowPass = rightChain.get<ChainPositions::lowPass>();
    updatePassFilter(rightLowPass, lowPassCoefficients, chainSettings.lowPassSlope);
}

void RuckusEQAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);
    updateHighPassFilters(chainSettings);
    updateBandPassFilter(chainSettings);
    updateLowPassFilters(chainSettings);
}

//sets up all of the configurable parameters in the plugin to be passed into the audio processor value tree state constructor.
juce::AudioProcessorValueTreeState::ParameterLayout
    RuckusEQAudioProcessor::createParameterLayout()
{
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
       
        juce::StringArray filterSlopes;
        for (int i = 0; i < 4; i++) {
            juce::String slope;
            slope << (12 + i*12);
            slope << " dB/Oct";
            filterSlopes.add(slope);
        }
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("HighPass Freq", 1),
                                                               "HighPass Freq",
                                                               juce::NormalisableRange<float>(10.f, 500.f, 1.f, 0.9f),
                                                               10.f));
        
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("HighPass Slope", 1), "HighPass Slope", filterSlopes, 0));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("LowPass Freq", 1),
                                                               "LowPass Freq",
                                                               juce::NormalisableRange<float>(3000.f, 21000.f, 1.f, 0.4f),
                                                               21000.f));
        
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("LowPass Slope", 1), "LowPass Slope", filterSlopes, 0));
        
        //Rumble
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Rumble Freq", 1),
                                                               "Rumble Freq",
                                                               juce::NormalisableRange<float>(20.f, 200.f, 1.f, 0.9f),
                                                               75.f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Rumble Gain", 1),
                                                               "Rumble Gain",
                                                               juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                               0.0f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Rumble Q", 1),
                                                               "Rumble Q",
                                                               juce::NormalisableRange<float>(0.1f, 3.4f, 0.05f, 1.f),
                                                               1.f));
        
        //Lows
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Low Freq", 1),
                                                               "Low Freq",
                                                               juce::NormalisableRange<float>(150.f, 400.f, 1.f, 0.85f),
                                                               250.f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Low Gain", 1),
                                                               "Low Gain",
                                                               juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                               0.0f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Low Q", 1),
                                                               "Low Q",
                                                               juce::NormalisableRange<float>(0.1f, 3.4f, 0.05f, 1.f),
                                                               1.f));
        
        //Low-mids
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("LowMid Freq", 1),
                                                               "LowMid Freq",
                                                               juce::NormalisableRange<float>(350.f, 1500.f, 1.f, 0.8f),
                                                               250.f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("LowMid Gain", 1),
                                                               "LowMid Gain",
                                                               juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                               0.0f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("LowMid Q", 1),
                                                               "LowMid Q",
                                                               juce::NormalisableRange<float>(0.1f, 3.4f, 0.05f, 1.f),
                                                               1.f));
        
        //High-mids
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("HighMid Freq", 1),
                                                               "HighMid Freq",
                                                               juce::NormalisableRange<float>(1000.f, 6000.f, 1.f, 0.65f),
                                                               250.f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("HighMid Gain", 1),
                                                               "HighMid Gain",
                                                               juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                               0.0f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("HighMid Q", 1),
                                                               "HighMid Q",
                                                               juce::NormalisableRange<float>(0.1f, 3.4f, 0.05f, 1.f),
                                                               1.f));
        
        //Highs
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("High Freq", 1),
                                                               "High Freq",
                                                               juce::NormalisableRange<float>(5000.f, 16000.f, 1.f, 0.5f),
                                                               250.f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("High Gain", 1),
                                                               "High Gain",
                                                               juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                               0.0f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("High Q", 1),
                                                               "High Q",
                                                               juce::NormalisableRange<float>(0.1f, 2.f, 0.05f, 1.f),
                                                               1.f));
        
        //Air
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Air Freq", 1),
                                                               "Air Freq",
                                                               juce::NormalisableRange<float>(14000.f, 22000.f, 1.f, 0.45f),
                                                               250.f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Air Gain", 1),
                                                               "Air Gain",
                                                               juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                               0.0f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Air Q", 1),
                                                               "Air Q",
                                                               juce::NormalisableRange<float>(0.1f, 1.2f, 0.05f, 1.f),
                                                               1.f));
        
        return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RuckusEQAudioProcessor();
}
