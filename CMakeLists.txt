cmake_minimum_required(VERSION 3.22)

project(voicevox_juce-demo VERSION 1.0.0)

#This is temporarily needed due to a bug in Xcode 15:
if (CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.0")
        add_compile_definitions(JUCE_SILENCE_XCODE_15_LINKER_WARNING=1)
        set(CMAKE_EXE_LINKER_FLAGS "-Wl,-ld_classic" CACHE INTERNAL "")
    endif ()
endif ()

#First, we'll add the CMake folder, incase we'll need to find_package later:
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

#Compile commands, useful for some IDEs like VS-Code
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

#Minimum MacOS target, set globally
if (${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0 CACHE STRING "Minimum OS X deployment version" FORCE)

    #code signing to run on an iOS device:
    #    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer" CACHE STRING "" FORCE)
    #    set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "XXXXXXXXXX" CACHE STRING "" FORCE)

    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
else ()
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version" FORCE)

    option(UniversalBinary "Build universal binary for mac" ON)

    if (UniversalBinary)
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")
    endif ()

    # From clap: If running into issues when Xcode tries to codesign the CLAP plugin, you may want to add these lines:
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO")
endif ()

# Static linking in Windows
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# For building in multibyte language environment.
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/utf-8)
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    # Refer to: https://cmake.org/cmake/help/latest/prop_tgt/MACOSX_RPATH.html
    set(CMAKE_SKIP_BUILD_RPATH  FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH "@loader_path")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # Refer to: https://stackoverflow.com/questions/57915564/cmake-how-to-set-rpath-to-origin-with-cmake
    set(CMAKE_SKIP_BUILD_RPATH  FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH "$\{ORIGIN\}")
endif()

# We 'require' that we need juce. If JUCE isn't found, it will revert to what's in
# CMake/Findjuce.cmake, where you can see how JUCE is brought in/configured
find_package(juce REQUIRED)

find_package(voicevox_juce REQUIRED)

juce_add_module(voicevox_juce_extra ALIAS_NAMESPACE cocotone)

juce_add_module(cocotone_song_editor_basics ALIAS_NAMESPACE cocotone)

# Add plugin project
add_subdirectory(AudioPlugin)
