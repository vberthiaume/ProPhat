#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

static const NormalisableRange<float> sliderRange = {-12.0f, 12.0f};

namespace sBMP4AudioProcessorIDs
{
    const String oscillatorsComboID = "oscillatorsCombo";
    const String oscillatorsSliderID = "oscillatorsSlider";
    const String oscillatorsButtonID = "oscillatorsButton";

    const String filterSliderID = "filterSlider";
    const String filterButtonID = "filterButton";

    const String envelopeSliderID = "envelopeSlider";
    const String envelopeButtonID = "envelopeButton";

    const String lfoComboID = "lfoCombo";

    const String lfoSliderID = "lfoSlider";

    const String oscillatorsSectionID = "oscillatorsSection";
    const String filterSectionID = "filterSection";
    const String envelopeSectionID = "envelopeSection";
    const String lfoSectionID = "lfoSection";

    const String roomSizeID = "roomSize";
}

namespace sBMP4AudioProcessorNames
{
    const String oscillatorsComboDescription = "Oscillators Model";
    const String oscillatorsSliderDescription = "Oscillators Gain";
    const String oscillatorsButtonDescription = "Oscillators ON";

    const String filterSliderDescription = "Filter Gain";
    const String filterButtonDescription = "Filter ON";

    const String envelopeSliderDescription = "Envelope Gain";
    const String envelopeButtonDescription = "Envelope ON";

    const String lfoComboDescription = "Wave Type";

    const String lfoSliderDescription = "LFO Frequency";

    const String oscillatorsSectionDescription = "Oscillators";
    const String filterSectionDescription = "Filter";
    const String envelopeSectionDescription = "Envelope";
    const String lfoSectionDescription = "LFO";

    const String roomSizeDescription = "Room Size";
}

namespace sBMP4AudioProcessorChoices
{
    const String oscillatorsChoice0 = "Sine";
    const String oscillatorsChoice1 = "Other";

    const String lfoChoices0 = "Sine";
    const String lfoChoices1 = "Pulse";
}

//==============================================================================
struct SineWaveSound : public SynthesiserSound
{
    SineWaveSound() {}

    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
class SineWaveVoice : public SynthesiserVoice
{
public:
    SineWaveVoice() {}

    bool canPlaySound (SynthesiserSound* sound) override
    {
        return dynamic_cast<SineWaveSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity,
                    SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        currentAngle = 0.0;
        level = velocity * 0.15;
        tailOff = 0.0;

        auto cyclesPerSecond = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        auto cyclesPerSample = cyclesPerSecond / getSampleRate();

        angleDelta = cyclesPerSample * 2.0 * MathConstants<double>::pi;
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            if (tailOff == 0.0)
                tailOff = 1.0;
        }
        else
        {
            clearCurrentNote();
            angleDelta = 0.0;
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        if (angleDelta != 0.0)
        {
            if (tailOff > 0.0) // [7]
            {
                while (--numSamples >= 0)
                {
                    auto currentSample = (float) (std::sin (currentAngle) * level * tailOff);

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;

                    tailOff *= 0.99; // [8]

                    if (tailOff <= 0.005)
                    {
                        clearCurrentNote(); // [9]

                        angleDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                while (--numSamples >= 0) // [6]
                {
                    auto currentSample = (float) (std::sin (currentAngle) * level);

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;
                }
            }
        }
    }

protected:
    double currentAngle = 0.0, angleDelta = 0.0, level = 0.0, tailOff = 0.0;
};

//==============================================================================

class SineWaveTableVoice : public SineWaveVoice
{
public:
    SineWaveTableVoice (const AudioSampleBuffer& wavetableToUse)
        : wavetable (wavetableToUse),
        tableSize (wavetable.getNumSamples() - 1)
    {
        jassert (wavetable.getNumChannels() == 1);
    }

    void startNote (int midiNoteNumber, float velocity, SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        currentAngle = 0.0;
        level = velocity * 0.15;
        tailOff = 0.0;

        auto cyclesPerSecond = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        setFrequency ((float) cyclesPerSecond);
        auto cyclesPerSample = cyclesPerSecond / getSampleRate();

        angleDelta = cyclesPerSample * 2.0 * MathConstants<double>::pi;
    }
    void renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        if (angleDelta != 0.0)
        {
            if (tailOff > 0.0) // [7]
            {
                while (--numSamples >= 0)
                {
                    auto currentSample = (float) (getNextSample()/*std::sin (currentAngle)*/ * level * tailOff);

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;

                    tailOff *= 0.99; // [8]

                    if (tailOff <= 0.005)
                    {
                        clearCurrentNote(); // [9]

                        angleDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                while (--numSamples >= 0) // [6]
                {
                    auto currentSample = (float) (getNextSample()/*std::sin (currentAngle)*/  * level);

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;
                }
            }
        }
    }

    void setFrequency (float frequency)
    {
        auto tableSizeOverSampleRate = tableSize / getSampleRate();
        tableDelta = frequency * (float) tableSizeOverSampleRate;
    }

    forcedinline float getNextSample() noexcept
    {
        auto index0 = (unsigned int) currentIndex;
        auto index1 = index0 + 1;

        auto frac = currentIndex - (float) index0;

        auto* table = wavetable.getReadPointer (0);
        auto value0 = table[index0];
        auto value1 = table[index1];

        auto currentSample = value0 + frac * (value1 - value0);

        if ((currentIndex += tableDelta) > tableSize)
            currentIndex -= tableSize;

        return currentSample;
    }

private:
    const AudioSampleBuffer& wavetable;
    const int tableSize;
    float currentIndex = 0.0f, tableDelta = 0.0f;
};


//==============================================================================

class sBMP4AudioProcessor : public AudioProcessor, public ValueTree::Listener
{
public:

    //==============================================================================
    sBMP4AudioProcessor();
    ~sBMP4AudioProcessor();

    void valueTreePropertyChanged (juce::ValueTree &/*v*/, const juce::Identifier &/*id*/) override
    {
    }

    float getSliderLinearGain (StringRef id)
    {
        return Decibels::decibelsToGain (sliderRange.convertFrom0to1 (state.getParameter (id)->getValue()));
    }

    void reset() override
    {
        lrFilter.reset();
    }

    bool getIsButtonEnabled (StringRef id) { return state.getParameter (id)->getValue(); }

    String getSelectedChoice (StringRef id) { return ((AudioParameterChoice*) state.getParameter (id))->getCurrentValueAsText(); }

    void valueTreeChildAdded (juce::ValueTree &/*parentTree*/, juce::ValueTree &/*childWhichHasBeenAdded*/) override {}
    void valueTreeChildRemoved (juce::ValueTree &/*parentTree*/, juce::ValueTree &/*childWhichHasBeenRemoved*/, int /*indexFromWhichChildWasRemoved*/) override {}
    void valueTreeChildOrderChanged (juce::ValueTree &/*parentTreeWhoseChildrenHaveMoved*/, int /*oldIndex*/, int /*newIndex*/) override {}
    void valueTreeParentChanged (juce::ValueTree &/*treeWhoseParentHasChanged*/) override {}

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
    void processBlock (AudioBuffer<double>&, MidiBuffer&) override;

    void process (AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
    void processLeftRight (AudioBuffer<float>& ogBuffer);

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    AudioProcessorValueTreeState state;

private:

    void createWavetable()
    {
        sineTable.setSize (1, tableSize + 1);
        sineTable.clear();

        auto* samples = sineTable.getWritePointer (0);

        //int harmonics[] = {1, 3, 5, 6, 7, 9, 13, 15};
        //float harmonicWeights[] = {0.5f, 0.1f, 0.05f, 0.125f, 0.09f, 0.005f, 0.002f, 0.001f};

        int harmonics[] = {1};
        float harmonicWeights[] = {1.f};

        jassert (numElementsInArray (harmonics) == numElementsInArray (harmonicWeights));

        for (auto harmonic = 0; harmonic < numElementsInArray (harmonics); ++harmonic)
        {
            auto angleDelta = MathConstants<double>::twoPi / (double) (tableSize - 1) * harmonics[harmonic];
            auto currentAngle = 0.0;

            for (size_t i = 0; i < tableSize; ++i)
            {
                auto sample = std::sin (currentAngle);
                samples[i] += (float) sample * harmonicWeights[harmonic];
                currentAngle += angleDelta;
            }
        }

        samples[tableSize] = samples[0];
    }

    //==============================================================================
    double lastSampleRate = {};

    Synthesiser synth;
    Reverb reverb;

    const unsigned int tableSize = 1 << 7;
    int numberOfOscillators = 16;
    AudioSampleBuffer sineTable;

    dsp::IIR::Filter<float> lrFilter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sBMP4AudioProcessor)
};
