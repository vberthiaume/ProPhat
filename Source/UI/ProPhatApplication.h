/*
  ==============================================================================

   Copyright (c) 2024 - Vincent Berthiaume

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   ProPhat IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

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

    void initialise (const juce::String&) override;

    void shutdown () override;

    void systemRequestedQuit () override;

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
