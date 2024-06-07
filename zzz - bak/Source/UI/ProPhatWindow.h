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

#include <JuceHeader.h>
#include "../submodules/JUCE/modules/juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h"
#include "../Utility/Macros.h"

/**
 * @brief The main window when running the plugin in standalone mode.
*/
class ProPhatWindow
    : public juce::DocumentWindow
#if USE_NATIVE_TITLE_BAR
    , public juce::DarkModeSettingListener
#else
    , private juce::Button::Listener
#endif
{
public:
    typedef juce::StandalonePluginHolder::PluginInOuts PluginInOuts;

    /** Creates a window with a given title and colour.
        The settings object can be a PropertySet that the class should use to
        store its settings (it can also be null). If takeOwnershipOfSettings is
        true, then the settings object will be owned and deleted by this object.
    */
    ProPhatWindow (const juce::String& title,
                  juce::Colour backgroundColour,
                  juce::PropertySet* settingsToUse,
                  bool takeOwnershipOfSettings,
                  const juce::String& preferredDefaultDeviceName = juce::String (),
                  const juce::AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions = nullptr,
                  const juce::Array<PluginInOuts>& constrainToConfiguration = {},
                  bool autoOpenMidiDevices = true
    );

    ~ProPhatWindow () override;
    void closeButtonPressed() override;
    void resized() override;

    juce::AudioProcessor* getAudioProcessor() const noexcept      { return pluginHolder->processor.get(); }
    juce::AudioDeviceManager& getDeviceManager() const noexcept   { return pluginHolder->deviceManager; }

    /** Deletes and re-creates the plugin, resetting it to its default state. */
    void resetToDefaultState();

#if USE_NATIVE_TITLE_BAR
    void darkModeSettingChanged () override;
#else
    static void menuCallback (int result, ProPhatWindow* button)
    {
        if (button != nullptr && result != 0)
            button->handleMenuResult (result);
    }
    void handleMenuResult (int result);
#endif

    virtual juce::StandalonePluginHolder* getPluginHolder()    { return pluginHolder.get(); }

    std::unique_ptr<juce::StandalonePluginHolder> pluginHolder;

private:
    void updateContent();

#if ! USE_NATIVE_TITLE_BAR
    void buttonClicked (juce::Button*) override;
#endif

    //==============================================================================
    class MainContentComponent  : public juce::Component,
                                  private juce::Value::Listener,
                                  private juce::Button::Listener,
                                  private juce::ComponentListener
    {
    public:
        MainContentComponent (ProPhatWindow& filterWindow)
            : owner (filterWindow), notification (this),
              editor (owner.getAudioProcessor()->hasEditor() ? owner.getAudioProcessor()->createEditorIfNeeded()
                                                             : new juce::GenericAudioProcessorEditor (*owner.getAudioProcessor()))
        {
            inputMutedValue.referTo (owner.pluginHolder->getMuteInputValue());

            if (editor != nullptr)
            {
                editor->addComponentListener (this);
                handleMovedOrResized();

                addAndMakeVisible (editor.get());
            }

            addChildComponent (notification);

            if (owner.pluginHolder->getProcessorHasPotentialFeedbackLoop())
            {
                inputMutedValue.addListener (this);
                shouldShowNotification = inputMutedValue.getValue();
            }

            inputMutedChanged (shouldShowNotification);
        }

        ~MainContentComponent() override
        {
            if (editor != nullptr)
            {
                editor->removeComponentListener (this);
                owner.pluginHolder->processor->editorBeingDeleted (editor.get());
                editor = nullptr;
            }
        }

        void resized() override
        {
            handleResized();
        }

        juce::ComponentBoundsConstrainer* getEditorConstrainer() const
        {
            if (auto* e = editor.get())
                return e->getConstrainer();

            return nullptr;
        }

        juce::BorderSize<int> computeBorder() const
        {
            const auto nativeFrame = [&]() -> juce::BorderSize<int>
            {
                if (auto* peer = owner.getPeer())
                    if (const auto frameSize = peer->getFrameSizeIfPresent())
                        return *frameSize;

                return {};
            }();

            return nativeFrame.addedTo (owner.getContentComponentBorder())
                              .addedTo (juce::BorderSize<int> { shouldShowNotification ? NotificationArea::height : 0, 0, 0, 0 });
        }

    private:
        //==============================================================================
        class NotificationArea : public juce::Component
        {
        public:
            enum { height = 30 };

            NotificationArea (juce::Button::Listener* settingsButtonListener)
                : notification ("notification", "Audio input is muted to avoid feedback loop"),
                 #if JUCE_IOS || JUCE_ANDROID
                  settingsButton ("Unmute Input")
                 #else
                  settingsButton ("Settings...")
                 #endif
            {
                setOpaque (true);

                notification.setColour (juce::Label::textColourId, juce::Colours::black);

                settingsButton.addListener (settingsButtonListener);

                addAndMakeVisible (notification);
                addAndMakeVisible (settingsButton);
            }

            void paint (juce::Graphics& g) override
            {
                auto r = getLocalBounds();

                g.setColour (juce::Colours::darkgoldenrod);
                g.fillRect (r.removeFromBottom (1));

                g.setColour (juce::Colours::lightgoldenrodyellow);
                g.fillRect (r);
            }

            void resized() override
            {
                auto r = getLocalBounds().reduced (5);

                settingsButton.setBounds (r.removeFromRight (70));
                notification.setBounds (r);
            }
        private:
            juce::Label notification;
            juce::TextButton settingsButton;
        };

        //==============================================================================
        void inputMutedChanged (bool newInputMutedValue)
        {
            shouldShowNotification = newInputMutedValue;
            notification.setVisible (shouldShowNotification);

           #if JUCE_IOS || JUCE_ANDROID
            handleResized();
           #else
            if (editor != nullptr)
            {
                const int extraHeight = shouldShowNotification ? NotificationArea::height : 0;
                const auto rect = getSizeToContainEditor();
                setSize (rect.getWidth(), rect.getHeight() + extraHeight);
            }
           #endif
        }

        void valueChanged (juce::Value& value) override     { inputMutedChanged (value.getValue()); }
        void buttonClicked (juce::Button*) override
        {
           #if JUCE_IOS || JUCE_ANDROID
            owner.pluginHolder->getMuteInputValue().setValue (false);
           #else
            owner.pluginHolder->showAudioSettingsDialog();
           #endif
        }

        //==============================================================================
        void handleResized()
        {
            auto r = getLocalBounds();

            if (shouldShowNotification)
                notification.setBounds (r.removeFromTop (NotificationArea::height));

            if (editor != nullptr)
            {
                const auto newPos = r.getTopLeft().toFloat().transformedBy (editor->getTransform().inverted());

                if (preventResizingEditor)
                    editor->setTopLeftPosition (newPos.roundToInt());
                else
                    editor->setBoundsConstrained (editor->getLocalArea (this, r.toFloat()).withPosition (newPos).toNearestInt());
            }
        }

        void handleMovedOrResized()
        {
            const juce::ScopedValueSetter<bool> scope (preventResizingEditor, true);

            if (editor != nullptr)
            {
                auto rect = getSizeToContainEditor();

                setSize (rect.getWidth(),
                         rect.getHeight() + (shouldShowNotification ? NotificationArea::height : 0));
            }
        }

        void componentMovedOrResized (Component&, bool, bool) override
        {
            handleMovedOrResized();
        }

        juce::Rectangle<int> getSizeToContainEditor() const
        {
            if (editor != nullptr)
                return getLocalArea (editor.get(), editor->getLocalBounds());

            return {};
        }

        //==============================================================================
        ProPhatWindow& owner;
        NotificationArea notification;
        std::unique_ptr<juce::AudioProcessorEditor> editor;
        juce::Value inputMutedValue;
        bool shouldShowNotification = false;
        bool preventResizingEditor = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
    };

    /*  This custom constrainer checks with the AudioProcessorEditor (which might itself be
        constrained) to ensure that any size we choose for the standalone window will be suitable
        for the editor too.

        Without this constrainer, attempting to resize the standalone window may set bounds on the
        peer that are unsupported by the inner editor. In this scenario, the peer will be set to a
        'bad' size, then the inner editor will be resized. The editor will check the new bounds with
        its own constrainer, and may set itself to a more suitable size. After that, the resizable
        window will see that its content component has changed size, and set the bounds of the peer
        accordingly. The end result is that the peer is resized twice in a row to different sizes,
        which can appear glitchy/flickery to the user.
    */
    class DecoratorConstrainer : public juce::BorderedComponentBoundsConstrainer
    {
    public:
        juce::ComponentBoundsConstrainer* getWrappedConstrainer() const override
        {
            return contentComponent != nullptr ? contentComponent->getEditorConstrainer() : nullptr;
        }

        juce::BorderSize<int> getAdditionalBorder() const override
        {
            return contentComponent != nullptr ? contentComponent->computeBorder() : juce::BorderSize<int>{};
        }

        void setMainContentComponent (MainContentComponent* in) { contentComponent = in; }

    private:
        MainContentComponent* contentComponent = nullptr;
    };

#if ! USE_NATIVE_TITLE_BAR
    juce::TextButton optionsButton;
#endif
    DecoratorConstrainer decoratorConstrainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProPhatWindow)
};
