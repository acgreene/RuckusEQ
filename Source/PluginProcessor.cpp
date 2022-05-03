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
void RuckusEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
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

void RuckusEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
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

//sets up all of the configurable parameters in the plugin to be passed into the audio processor value tree state constructor.
juce::AudioProcessorValueTreeState::ParameterLayout
    RuckusEQAudioProcessor::createParameterLayout()
{
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
       
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("HighPass Freq", 1),
                                                               "HighPass Freq",
                                                               juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                               20.f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("LowPass Freq", 1),
                                                               "LowPass Freq",
                                                               juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                               20000.f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("BandPass Freq", 1),
                                                               "BandPass Freq",
                                                               juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
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
