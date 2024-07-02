#including CPM.cmake, a package manager:
#https://github.com/TheLartians/CPM.cmake
include(CPM)

set(CPM_voicevox_juce_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/voicevox_juce")
CPMAddPackage(NAME "voicevox_juce" SOURCE_DIR ${CPM_voicevox_juce_SOURCE})
