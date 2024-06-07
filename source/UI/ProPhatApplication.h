/*
  ==============================================================================

    ProPhat is a virtual synthesizer inspired by the Prophet REV2.
    Copyright (C) 2024 Vincent Berthiaume

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

#pragma once

#include "ProPhatWindow.h"

class ProPhatApplication final : public juce::JUCEApplication
{
public:
    ProPhatApplication ();

    const juce::String getApplicationName () override { return appName; }
    const juce::String getApplicationVersion () override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed () override { return true; }
    void anotherInstanceStarted (const juce::String&) override {}

    virtual ProPhatWindow* createWindow ();

    juce::StandalonePluginHolder* getPluginHolder() { return mainWindow ? mainWindow->getPluginHolder() : nullptr; }

    void initialise (const juce::String&) override;

    void shutdown () override;

    void systemRequestedQuit () override;

    void resetToDefaultState () { if (mainWindow) mainWindow->resetToDefaultState (); }

protected:
    juce::ApplicationProperties appProperties;
    std::unique_ptr<ProPhatWindow> mainWindow;

private:
    const juce::String appName { juce::CharPointer_UTF8 (JucePlugin_Name) };
};

#if JucePlugin_Build_Standalone && JUCE_IOS

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wmissing-prototypes")

using namespace juce;

bool JUCE_CALLTYPE juce_isInterAppAudioConnected ()
{
    if (auto holder = StandalonePluginHolder::getInstance ())
        return holder->isInterAppAudioConnected ();

    return false;
}

void JUCE_CALLTYPE juce_switchToHostApplication ()
{
    if (auto holder = juce::StandalonePluginHolder::getInstance ())
        holder->switchToHostApplication ();
}

Image JUCE_CALLTYPE juce_getIAAHostIcon (int size)
{
    if (auto holder = StandalonePluginHolder::getInstance ())
        return holder->getIAAHostIcon (size);

    return Image ();
}

JUCE_END_IGNORE_WARNINGS_GCC_LIKE

#endif
