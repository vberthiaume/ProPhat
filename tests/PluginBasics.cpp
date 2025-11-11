#include "helpers/test_helpers.h"
#include <DSP/ProPhatProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

TEST_CASE ("one is equal to one", "[dummy]")
{
    REQUIRE (1 == 1);
}

TEST_CASE ("Plugin instance", "[instance]")
{
    ProPhatProcessor testPlugin;

    SECTION ("name")
    {
        CHECK_THAT (testPlugin.getName().toStdString(),
            Catch::Matchers::Equals ("ProPhat"));
    }
}

TEST_CASE ("processBlock generates audio from MIDI input", "[processBlock]")
{
    ProPhatProcessor processor;

    const double sampleRate = 48000.0;
    const int    blockSize  = 128;

    // --- FLOAT PROCESSING ---
    SECTION ("float buffer")
    {
        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer         midi;

        // Add a simple MIDI note-on + note-off pair
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
        midi.addEvent (juce::MidiMessage::noteOff (1, 60), blockSize / 2);

        buffer.clear();
        processor.prepareToPlay (sampleRate, blockSize);

        REQUIRE_FALSE (processor.isUsingDoublePrecision());

        REQUIRE_NOTHROW (processor.processBlock (buffer, midi));

        // The buffer should now contain finite values (could be silence if envelope not yet triggered)
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                REQUIRE (std::isfinite (buffer.getSample (ch, i)));
    }

    // --- DOUBLE PROCESSING ---
    // SECTION ("double buffer")
    // {
    //     juce::AudioBuffer<double> buffer (2, blockSize);
    //     juce::MidiBuffer          midi;

    //     midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
    //     midi.addEvent (juce::MidiMessage::noteOff (1, 60), blockSize / 2);

    //     buffer.clear();

    //     processor.setProcessingPrecision (juce::AudioProcessor::doublePrecision);
    //     processor.prepareToPlay (sampleRate, blockSize);

    //     REQUIRE (processor.isUsingDoublePrecision());

    //     REQUIRE_NOTHROW (processor.processBlock (buffer, midi));

    //     for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    //         for (int i = 0; i < buffer.getNumSamples(); ++i)
    //             REQUIRE (std::isfinite (buffer.getSample (ch, i)));
    // }
}

#ifdef PAMPLEJUCE_IPP
    #include <ipp.h>

TEST_CASE ("IPP version", "[ipp]")
{
    CHECK_THAT (ippsGetLibVersion()->Version, Catch::Matchers::Equals ("2022.0.0 (r0x131e93b0)"));
}
#endif
