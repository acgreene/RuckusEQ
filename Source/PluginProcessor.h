/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class RuckusEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    RuckusEQAudioProcessor(); //constructor
    ~RuckusEQAudioProcessor() override; //destructor

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    //provides a list of all parameters and their attributes within the plugin.
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    //contains a value tree that is used to manage an audio processors entire state. connects audio parameters to gui.
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};

private:
    //create Filter type alias to make code cleaner
    //filter has a response of 12 dB/Oct when it's configured as a HPF or LPF
    using Filter = juce::dsp::IIR::Filter<float>;
    
    //to do dsp in juce we need to create a series of processing, defined as a processing chain, and then pass a context through it.
    //if we use four 12 dB/Oct filters, we can create a 48 dB/Oct filter.
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    
    //the entire mono signal path is HPF -> BPF -> LPF
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    
    //two instances of mono to create stereo.
    MonoChain leftChain, rightChain;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RuckusEQAudioProcessor)
};
