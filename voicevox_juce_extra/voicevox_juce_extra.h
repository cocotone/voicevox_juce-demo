/*******************************************************************************
 The block below describes the properties of this module, and is read by
 the Projucer to automatically generate project code that uses it.
 For details about the syntax and how to create or use a module, see the
 JUCE Module Format.md file.


 BEGIN_JUCE_MODULE_DECLARATION

  ID:                 voicevox_juce_extra
  vendor:             COCOTONE
  version:            0.0.1
  name:               JUCE extended classes for integrating with voicevox_juce module.
  description:        Classes for creating advanced VOICEVOX application with JUCE framework.
  website:            https://github.com/cocotone/voicevox_juce_extra
  license:            BSD 3-Clause License
  minimumCppStandard: 17

  dependencies:       juce_core juce_audio_basics juce_gui_basics

 END_JUCE_MODULE_DECLARATION

*******************************************************************************/


#pragma once
#define VOICEVOX_JUCE_EXTRA_H_INCLUDED

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================

#include "Engine/VoicevoxEngine.h"
