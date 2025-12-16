/*
==============================================================================

    ProPhat is a virtual synthesizer inspired by the Prophet REV2.
    Copyright (C) 2025 Vincent Berthiaume

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

  ==============================================================================
*/

#include "LockFreeSynthesiser.h"
#include "juce_events/juce_events.h"

bool LockFreeSynthesiserVoice::isPlayingChannel (const int midiChannel) const
{
    return currentPlayingMidiChannel == midiChannel;
}

void LockFreeSynthesiserVoice::setCurrentPlaybackSampleRate (const double newRate)
{
    currentSampleRate = newRate;
}

bool LockFreeSynthesiserVoice::isVoiceActive() const
{
    return getCurrentlyPlayingNote() >= 0;
}

void LockFreeSynthesiserVoice::clearCurrentNote()
{
    currentlyPlayingNote      = -1;
    currentlyPlayingSound     = nullptr;
    currentPlayingMidiChannel = 0;
}

bool LockFreeSynthesiserVoice::wasStartedBefore (const LockFreeSynthesiserVoice& other) const noexcept
{
    return noteOnTime < other.noteOnTime;
}

void LockFreeSynthesiserVoice::renderNextBlock (juce::AudioBuffer<double>& outputBuffer, int startSample, int numSamples)
{
    juce::AudioBuffer<double> subBuffer (outputBuffer.getArrayOfWritePointers(), outputBuffer.getNumChannels(), startSample, numSamples);

    tempBuffer.makeCopyOf (subBuffer, true);
    renderNextBlock (tempBuffer, 0, numSamples);
    subBuffer.makeCopyOf (tempBuffer, true);
}

//======================================================

LockFreeSynthesiser::LockFreeSynthesiser()
{
    for (int i = 0; i < juce::numElementsInArray (lastPitchWheelValues); ++i)
        lastPitchWheelValues[i] = 0x2000;
}

LockFreeSynthesiserVoice* LockFreeSynthesiser::addVoice (LockFreeSynthesiserVoice* const newVoice)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    newVoice->setCurrentPlaybackSampleRate (sampleRate);

    auto* voice = voices.add (newVoice);

    usableVoicesToStealArray.ensureStorageAllocated (voices.size() + 1);

    return voice;
}

juce::SynthesiserSound* LockFreeSynthesiser::addSound (const juce::SynthesiserSound::Ptr& newSound)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    return sounds.add (newSound);
}

void LockFreeSynthesiser::setNoteStealingEnabled (const bool shouldSteal)
{
    shouldStealNotes = shouldSteal;
}

void LockFreeSynthesiser::setMinimumRenderingSubdivisionSize (int numSamples, bool shouldBeStrict) noexcept
{
    jassert (numSamples > 0); // it wouldn't make much sense for this to be less than 1
    minimumSubBlockSize = numSamples;
    subBlockSubdivisionIsStrict = shouldBeStrict;
}

//==============================================================================
void LockFreeSynthesiser::setCurrentPlaybackSampleRate (const double newRate)
{
    if (! juce::approximatelyEqual (sampleRate, newRate))
    {
        allNotesOff (0, false);
        sampleRate = newRate;

        for (auto* voice : voices)
            voice->setCurrentPlaybackSampleRate (newRate);
    }
}

template <typename floatType>
void LockFreeSynthesiser::processNextBlock (juce::AudioBuffer<floatType>& outputAudio,
                                    const juce::MidiBuffer& midiData,
                                    int startSample,
                                    int numSamples)
{
    // must set the sample rate before using this!
    jassert (! juce::exactlyEqual (sampleRate, 0.0));
    const int targetChannels = outputAudio.getNumChannels();

    auto midiIterator = midiData.findNextSamplePosition (startSample);

    bool firstEvent = true;

    for (; numSamples > 0; ++midiIterator)
    {
        if (midiIterator == midiData.cend())
        {
            if (targetChannels > 0)
                renderVoices (outputAudio, startSample, numSamples);

            return;
        }

        const auto metadata = *midiIterator;
        const int samplesToNextMidiMessage = metadata.samplePosition - startSample;

        if (samplesToNextMidiMessage >= numSamples)
        {
            if (targetChannels > 0)
                renderVoices (outputAudio, startSample, numSamples);

            handleMidiEvent (metadata.getMessage());
            break;
        }

        if (samplesToNextMidiMessage < ((firstEvent && ! subBlockSubdivisionIsStrict) ? 1 : minimumSubBlockSize))
        {
            handleMidiEvent (metadata.getMessage());
            continue;
        }

        firstEvent = false;

        if (targetChannels > 0)
            renderVoices (outputAudio, startSample, samplesToNextMidiMessage);

        handleMidiEvent (metadata.getMessage());
        startSample += samplesToNextMidiMessage;
        numSamples  -= samplesToNextMidiMessage;
    }

    std::for_each (midiIterator,
                   midiData.cend(),
                   [&] (const juce::MidiMessageMetadata& meta) { handleMidiEvent (meta.getMessage()); });
}

// explicit template instantiation
template void LockFreeSynthesiser::processNextBlock<float>  (juce::AudioBuffer<float>&,  const juce::MidiBuffer&, int, int);
template void LockFreeSynthesiser::processNextBlock<double> (juce::AudioBuffer<double>&, const juce::MidiBuffer&, int, int);

void LockFreeSynthesiser::renderNextBlock (juce::AudioBuffer<float>& outputAudio, const juce::MidiBuffer& inputMidi,
                                   int startSample, int numSamples)
{
    processNextBlock (outputAudio, inputMidi, startSample, numSamples);
}

void LockFreeSynthesiser::renderNextBlock (juce::AudioBuffer<double>& outputAudio, const juce::MidiBuffer& inputMidi,
                                   int startSample, int numSamples)
{
    processNextBlock (outputAudio, inputMidi, startSample, numSamples);
}

void LockFreeSynthesiser::renderVoices (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    for (auto* voice : voices)
        voice->renderNextBlock (buffer, startSample, numSamples);
}

void LockFreeSynthesiser::renderVoices (juce::AudioBuffer<double>& buffer, int startSample, int numSamples)
{
    for (auto* voice : voices)
        voice->renderNextBlock (buffer, startSample, numSamples);
}

void LockFreeSynthesiser::handleMidiEvent (const juce::MidiMessage& m)
{
    const int channel = m.getChannel();

    if (m.isNoteOn())
    {
        noteOn (channel, m.getNoteNumber(), m.getFloatVelocity());
    }
    else if (m.isNoteOff())
    {
        noteOff (channel, m.getNoteNumber(), m.getFloatVelocity(), true);
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        allNotesOff (channel, true);
    }
    else if (m.isPitchWheel())
    {
        const int wheelPos = m.getPitchWheelValue();
        lastPitchWheelValues [channel - 1] = wheelPos;
        handlePitchWheel (channel, wheelPos);
    }
    else if (m.isAftertouch())
    {
        handleAftertouch (channel, m.getNoteNumber(), m.getAfterTouchValue());
    }
    else if (m.isChannelPressure())
    {
        handleChannelPressure (channel, m.getChannelPressureValue());
    }
    else if (m.isController())
    {
        handleController (channel, m.getControllerNumber(), m.getControllerValue());
    }
    else if (m.isProgramChange())
    {
        handleProgramChange (channel, m.getProgramChangeNumber());
    }
}

//==============================================================================
void LockFreeSynthesiser::noteOn (const int midiChannel,
                          const int midiNoteNumber,
                          const float velocity)
{
    for (auto* sound : sounds)
    {
        if (sound->appliesToNote (midiNoteNumber) && sound->appliesToChannel (midiChannel))
        {
            // If hitting a note that's still ringing, stop it first (it could be
            // still playing because of the sustain or sostenuto pedal).
            for (auto* voice : voices)
                if (voice->getCurrentlyPlayingNote() == midiNoteNumber && voice->isPlayingChannel (midiChannel))
                    stopVoice (voice, 1.0f, true);

            startVoice (findFreeVoice (sound, midiChannel, midiNoteNumber, shouldStealNotes),
                        sound, midiChannel, midiNoteNumber, velocity);
        }
    }
}

void LockFreeSynthesiser::startVoice (LockFreeSynthesiserVoice* const voice,
                              juce::SynthesiserSound* const sound,
                              const int midiChannel,
                              const int midiNoteNumber,
                              const float velocity)
{
    if (voice != nullptr && sound != nullptr)
    {
        if (voice->currentlyPlayingSound != nullptr)
            voice->stopNote (0.0f, false);

        voice->currentlyPlayingNote = midiNoteNumber;
        voice->currentPlayingMidiChannel = midiChannel;
        voice->noteOnTime = ++lastNoteOnCounter;
        voice->currentlyPlayingSound = sound;
        voice->setKeyDown (true);
        voice->setSostenutoPedalDown (false);
        voice->setSustainPedalDown (sustainPedalsDown[midiChannel]);

        voice->startNote (midiNoteNumber, velocity, sound,
                          lastPitchWheelValues [midiChannel - 1]);
    }
}

void LockFreeSynthesiser::stopVoice (LockFreeSynthesiserVoice* voice, float velocity, const bool allowTailOff)
{
    if (voice == nullptr)
    {
        jassertfalse;
        return;
    }

    voice->stopNote (velocity, allowTailOff);

    // the subclass MUST call clearCurrentNote() if it's not tailing off! RTFM for stopNote()!
    jassert (allowTailOff || (voice->getCurrentlyPlayingNote() < 0 && voice->getCurrentlyPlayingSound() == nullptr));
}

void LockFreeSynthesiser::noteOff (const int midiChannel,
                           const int midiNoteNumber,
                           const float velocity,
                           const bool allowTailOff)
{
    for (auto* voice : voices)
    {
        if (voice->getCurrentlyPlayingNote() == midiNoteNumber
              && voice->isPlayingChannel (midiChannel))
        {
            if (auto sound = voice->getCurrentlyPlayingSound())
            {
                if (sound->appliesToNote (midiNoteNumber)
                     && sound->appliesToChannel (midiChannel))
                {
                    jassert (! voice->keyIsDown || voice->isSustainPedalDown() == sustainPedalsDown [midiChannel]);

                    voice->setKeyDown (false);

                    if (! (voice->isSustainPedalDown() || voice->isSostenutoPedalDown()))
                        stopVoice (voice, velocity, allowTailOff);
                }
            }
        }
    }
}

void LockFreeSynthesiser::allNotesOff (const int midiChannel, const bool allowTailOff)
{
    for (auto* voice : voices)
        if (midiChannel <= 0 || voice->isPlayingChannel (midiChannel))
            voice->stopNote (1.0f, allowTailOff);

    sustainPedalsDown.clear();
}

void LockFreeSynthesiser::handlePitchWheel (const int midiChannel, const int wheelValue)
{
    for (auto* voice : voices)
        if (midiChannel <= 0 || voice->isPlayingChannel (midiChannel))
            voice->pitchWheelMoved (wheelValue);
}

void LockFreeSynthesiser::handleController (const int midiChannel,
                                    const int controllerNumber,
                                    const int controllerValue)
{
    switch (controllerNumber)
    {
        case 0x40:  handleSustainPedal   (midiChannel, controllerValue >= 64); break;
        case 0x42:  handleSostenutoPedal (midiChannel, controllerValue >= 64); break;
        case 0x43:  handleSoftPedal      (midiChannel, controllerValue >= 64); break;
        default:    break;
    }

    for (auto* voice : voices)
        if (midiChannel <= 0 || voice->isPlayingChannel (midiChannel))
            voice->controllerMoved (controllerNumber, controllerValue);
}

void LockFreeSynthesiser::handleAftertouch (int midiChannel, int midiNoteNumber, int aftertouchValue)
{
    for (auto* voice : voices)
        if (voice->getCurrentlyPlayingNote() == midiNoteNumber
              && (midiChannel <= 0 || voice->isPlayingChannel (midiChannel)))
            voice->aftertouchChanged (aftertouchValue);
}

void LockFreeSynthesiser::handleChannelPressure (int midiChannel, int channelPressureValue)
{
    for (auto* voice : voices)
        if (midiChannel <= 0 || voice->isPlayingChannel (midiChannel))
            voice->channelPressureChanged (channelPressureValue);
}

void LockFreeSynthesiser::handleSustainPedal (int midiChannel, bool isDown)
{
    jassert (midiChannel > 0 && midiChannel <= 16);

    if (isDown)
    {
        sustainPedalsDown.setBit (midiChannel);

        for (auto* voice : voices)
            if (voice->isPlayingChannel (midiChannel) && voice->isKeyDown())
                voice->setSustainPedalDown (true);
    }
    else
    {
        for (auto* voice : voices)
        {
            if (voice->isPlayingChannel (midiChannel))
            {
                voice->setSustainPedalDown (false);

                if (! (voice->isKeyDown() || voice->isSostenutoPedalDown()))
                    stopVoice (voice, 1.0f, true);
            }
        }

        sustainPedalsDown.clearBit (midiChannel);
    }
}

void LockFreeSynthesiser::handleSostenutoPedal (int midiChannel, bool isDown)
{
    jassert (midiChannel > 0 && midiChannel <= 16);

    for (auto* voice : voices)
    {
        if (voice->isPlayingChannel (midiChannel))
        {
            if (isDown)
                voice->setSostenutoPedalDown (true);
            else if (voice->isSostenutoPedalDown())
                stopVoice (voice, 1.0f, true);
        }
    }
}

void LockFreeSynthesiser::handleSoftPedal ([[maybe_unused]] int midiChannel, bool /*isDown*/)
{
    jassert (midiChannel > 0 && midiChannel <= 16);
}

void LockFreeSynthesiser::handleProgramChange ([[maybe_unused]] int midiChannel,
                                       [[maybe_unused]] int programNumber)
{
    jassert (midiChannel > 0 && midiChannel <= 16);
}

//==============================================================================
LockFreeSynthesiserVoice* LockFreeSynthesiser::findFreeVoice (juce::SynthesiserSound* soundToPlay,
                                              int midiChannel, int midiNoteNumber,
                                              const bool stealIfNoneAvailable) const
{
    for (auto* voice : voices)
        if ((! voice->isVoiceActive()) && voice->canPlaySound (soundToPlay))
            return voice;

    if (stealIfNoneAvailable)
        return findVoiceToSteal (soundToPlay, midiChannel, midiNoteNumber);

    return nullptr;
}

LockFreeSynthesiserVoice* LockFreeSynthesiser::findVoiceToSteal (juce::SynthesiserSound* soundToPlay,
                                                 int /*midiChannel*/, int midiNoteNumber) const
{
    // This voice-stealing algorithm applies the following heuristics:
    // - Re-use the oldest notes first
    // - Protect the lowest & topmost notes, even if sustained, but not if they've been released.

    // apparently you are trying to render audio without having any voices...
    jassert (! voices.isEmpty());

    // These are the voices we want to protect (ie: only steal if unavoidable)
    LockFreeSynthesiserVoice* low = nullptr; // Lowest sounding note, might be sustained, but NOT in release phase
    LockFreeSynthesiserVoice* top = nullptr; // Highest sounding note, might be sustained, but NOT in release phase

    // this is a list of voices we can steal, sorted by how long they've been running
    usableVoicesToStealArray.clear();

    for (auto* voice : voices)
    {
        if (voice->canPlaySound (soundToPlay))
        {
            jassert (voice->isVoiceActive()); // We wouldn't be here otherwise

            usableVoicesToStealArray.add (voice);

            // NB: Using a functor rather than a lambda here due to scare-stories about
            // compilers generating code containing heap allocations.
            struct Sorter
            {
                bool operator() (const LockFreeSynthesiserVoice* a, const LockFreeSynthesiserVoice* b) const noexcept { return a->wasStartedBefore (*b); }
            };

            std::sort (usableVoicesToStealArray.begin(), usableVoicesToStealArray.end(), Sorter());

            if (! voice->isPlayingButReleased()) // Don't protect released notes
            {
                auto note = voice->getCurrentlyPlayingNote();

                if (low == nullptr || note < low->getCurrentlyPlayingNote())
                    low = voice;

                if (top == nullptr || note > top->getCurrentlyPlayingNote())
                    top = voice;
            }
        }
    }

    // Eliminate pathological cases (ie: only 1 note playing): we always give precedence to the lowest note(s)
    if (top == low)
        top = nullptr;

    // The oldest note that's playing with the target pitch is ideal.
    for (auto* voice : usableVoicesToStealArray)
        if (voice->getCurrentlyPlayingNote() == midiNoteNumber)
            return voice;

    // Oldest voice that has been released (no finger on it and not held by sustain pedal)
    for (auto* voice : usableVoicesToStealArray)
        if (voice != low && voice != top && voice->isPlayingButReleased())
            return voice;

    // Oldest voice that doesn't have a finger on it:
    for (auto* voice : usableVoicesToStealArray)
        if (voice != low && voice != top && ! voice->isKeyDown())
            return voice;

    // Oldest voice that isn't protected
    for (auto* voice : usableVoicesToStealArray)
        if (voice != low && voice != top)
            return voice;

    // We've only got "protected" voices now: lowest note takes priority
    jassert (low != nullptr);

    // Duophonic synth: give priority to the bass note:
    if (top != nullptr)
        return top;

    return low;
}