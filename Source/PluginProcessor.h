/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

// extract parameters from audio processor value tree state, create a data structure representing all parameter values.
struct ChainSettings
{
    //Rumble 20Hz-200Hz
    float rumbleFreq { 0 }, rumbleGainInDecibels{ 0 }, rumbleQuality {1.f};
    
    //Lows 150Hz-400Hz
    float lowFreq { 0 }, lowGainInDecibels{ 0 }, lowQuality {1.f};
    
    //Low-Mids 0.35kHz-1.5kHz
    float lowMidFreq { 0 }, lowMidGainInDecibels{ 0 }, lowMidQuality {1.f};
    
    //High-Mids 1kHz-6kHz
    float highMidFreq { 0 }, highMidGainInDecibels{ 0 }, highMidQuality {1.f};
    
    //Highs 5kHz-16kHz
    float highFreq { 0 }, highGainInDecibels{ 0 }, highQuality {1.f};
    
    //Air 12kHz-22kHz
    float airFreq { 0 }, airGainInDecibels{ 0 }, airQuality {1.f};
    
    float highPassFreq { 0 }, lowPassFreq { 0 };
    Slope highPassSlope { Slope::Slope_12 }, lowPassSlope { Slope::Slope_12 };
};

// define helper function that will give us all parameter values in the data struct
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

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
    
    //the entire mono signal path is HPF -> rumble -> low -> lowMid -> highMid -> high -> air -> LPF
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, Filter, Filter, Filter, Filter, Filter, CutFilter>;
    
    //two instances of mono to create stereo.
    MonoChain leftChain, rightChain;
    
    enum ChainPositions
    {
        highPass,
        rumble,
        low,
        lowMid,
        highMid,
        high,
        air,
        lowPass
    };
    
    //functions below prevent repeating blocks of code in prepareToPlay and processBlock.
    void updateBandPassFilter(const ChainSettings& chainSettings);
    
    using Coefficients = Filter::CoefficientsPtr;
    static void updateCoefficients(Coefficients& old, const Coefficients &replacements);
    
    template<int index, typename ChainType, typename CoefficientType>
    void update(ChainType& chain, const CoefficientType& coefficients)
    {
        updateCoefficients(chain.template get<index>().coefficients, coefficients[index]);
        chain.template setBypassed<index>(false);
    }
    
    template<typename ChainType, typename CoefficientType>
    void updatePassFilter(ChainType& chain, const CoefficientType& coefficients, const Slope& slope)
    {
        chain.template setBypassed<0>(true);
        chain.template setBypassed<1>(true);
        chain.template setBypassed<2>(true);
        chain.template setBypassed<3>(true);
        
        switch(slope)
        {
            case Slope_48:
            {
                update<3>(chain, coefficients);
            }
            case Slope_36:
            {
                update<2>(chain, coefficients);
            }
            case Slope_24:
            {
                update<1>(chain, coefficients);
            }
            case Slope_12:
            {
                update<0>(chain, coefficients);
            }
        }
    }
    
    void updateHighPassFilters(const ChainSettings& chainSettings);
    void updateLowPassFilters(const ChainSettings& chainSettings);
    
    void updateFilters();
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RuckusEQAudioProcessor)
};
