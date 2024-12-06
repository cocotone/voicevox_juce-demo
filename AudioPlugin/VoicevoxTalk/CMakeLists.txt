cmake_minimum_required(VERSION 3.22)

#==============================================================

# Declare helper function.
function(assign_source_group)
    foreach(_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else()
            set(_source_rel "${_source}")
        endif()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
endfunction(assign_source_group)

#==============================================================

set(TARGET_NAME_INST VoicevoxTalk)

juce_add_plugin(${TARGET_NAME_INST}
    VERSION 1.0.0
    # ICON_BIG ...
    # ICON_SMALL ...
    COMPANY_NAME "COCOTONE"
    COMPANY_COPYRIGHT "Copyright 2024 COCOTONE, Inc."
    COMPANY_WEBSITE "https://www.cocotone.jp/"
    COMPANY_EMAIL "info@cocotone.jp"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Cctn
    PLUGIN_CODE Vvti
    PLUGIN_NAME "Voicevox Talk"
    FORMATS AU VST3 Standalone
    PRODUCT_NAME ${TARGET_NAME_INST}
    )

file (GLOB_RECURSE ${TARGET_NAME_INST}_source_list CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.hpp
    )

assign_source_group("${${TARGET_NAME_INST}_source_list}")

target_sources(${TARGET_NAME_INST}
    PRIVATE
        ${${TARGET_NAME_INST}_source_list}
    )

target_compile_definitions(${TARGET_NAME_INST}
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(${TARGET_NAME_INST}
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
        voicevox::voicevox_core
        voicevox::voicevox_juce
        cocotone::voicevox_juce_extra
        cocotone::cocotone_song_editor_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Due to windows dll search behaviour.
# VST3 plugin should delay load linked dlls.
# https://docs.analytica.com/index.php/Ana:Embedded_Python/Research_notes
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_link_options(${TARGET_NAME_INST} 
        PUBLIC
            /DELAYLOAD:voicevox_core.dll
    )
endif()

# Pre link and post build command process for Standalone...
if(TARGET ${TARGET_NAME_INST}_Standalone)
    add_custom_command(
        TARGET ${TARGET_NAME_INST}_Standalone
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::voicevox_core> $<TARGET_FILE_DIR:${TARGET_NAME_INST}_Standalone>
    )

    add_custom_command(
        TARGET ${TARGET_NAME_INST}_Standalone
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::onnxruntime> $<TARGET_FILE_DIR:${TARGET_NAME_INST}_Standalone>
    )

    if(TARGET voicevox::onnxruntime_providers_shared)
        add_custom_command(
            TARGET ${TARGET_NAME_INST}_Standalone
            PRE_LINK
            COMMAND
                ${CMAKE_COMMAND} -E
                copy $<TARGET_FILE:voicevox::onnxruntime_providers_shared> $<TARGET_FILE_DIR:${TARGET_NAME_INST}_Standalone>
        )
    endif()

    # Copy resource directory to build directory.
    add_custom_command(
        TARGET ${TARGET_NAME_INST}_Standalone
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_model_dir> $<TARGET_FILE_DIR:${TARGET_NAME_INST}_Standalone>/model
    )

    add_custom_command(
        TARGET ${TARGET_NAME_INST}_Standalone
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_open_jtalk_dic_dir> $<TARGET_FILE_DIR:${TARGET_NAME_INST}_Standalone>/open_jtalk_dic_utf_8
    )
endif()

# Pre link and post build command process for VST3...
if(TARGET ${TARGET_NAME_INST}_VST3)
    add_custom_command(
        TARGET ${TARGET_NAME_INST}_VST3
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::voicevox_core> $<TARGET_FILE_DIR:${TARGET_NAME_INST}_VST3>
    )

    add_custom_command(
        TARGET ${TARGET_NAME_INST}_VST3
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::onnxruntime> $<TARGET_FILE_DIR:${TARGET_NAME_INST}_VST3>
    )

    if(TARGET voicevox::onnxruntime_providers_shared)
        add_custom_command(
            TARGET ${TARGET_NAME_INST}_VST3
            PRE_LINK
            COMMAND
                ${CMAKE_COMMAND} -E
                copy $<TARGET_FILE:voicevox::onnxruntime_providers_shared> $<TARGET_FILE_DIR:${TARGET_NAME_INST}_VST3>
        )
    endif()

    # Copy resource directory to build directory.
    add_custom_command(
        TARGET ${TARGET_NAME_INST}_VST3
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_model_dir> $<TARGET_FILE_DIR:${TARGET_NAME_INST}_VST3>/model
    )

    add_custom_command(
        TARGET ${TARGET_NAME_INST}_VST3
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_open_jtalk_dic_dir> $<TARGET_FILE_DIR:${TARGET_NAME_INST}_VST3>/open_jtalk_dic_utf_8
    )
endif()

# Pre link and post build command process for VST3...
if(TARGET ${TARGET_NAME_INST}_AU)
    add_custom_command(
        TARGET ${TARGET_NAME_INST}_AU
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::voicevox_core> $<TARGET_FILE_DIR:${TARGET_NAME_INST}_AU>
    )

    add_custom_command(
        TARGET ${TARGET_NAME_INST}_AU
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::onnxruntime> $<TARGET_FILE_DIR:${TARGET_NAME_INST}_AU>
    )

    if(TARGET voicevox::onnxruntime_providers_shared)
        add_custom_command(
            TARGET ${TARGET_NAME_INST}_AU
            PRE_LINK
            COMMAND
                ${CMAKE_COMMAND} -E
                copy $<TARGET_FILE:voicevox::onnxruntime_providers_shared> $<TARGET_FILE_DIR:${TARGET_NAME_INST}_AU>
        )
    endif()

    # Copy resource directory to build directory.
    add_custom_command(
        TARGET ${TARGET_NAME_INST}_AU
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_model_dir> $<TARGET_FILE_DIR:${TARGET_NAME_INST}_AU>/model
    )

    add_custom_command(
        TARGET ${TARGET_NAME_INST}_AU
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_open_jtalk_dic_dir> $<TARGET_FILE_DIR:${TARGET_NAME_INST}_AU>/open_jtalk_dic_utf_8
    )
endif()

#==============================================================

set(TARGET_NAME_FX VoicevoxTalkFX)

juce_add_plugin(${TARGET_NAME_FX}
    VERSION 1.0.0
    # ICON_BIG ...
    # ICON_SMALL ...
    COMPANY_NAME "COCOTONE"
    COMPANY_COPYRIGHT "Copyright 2024 COCOTONE, Inc."
    COMPANY_WEBSITE "https://www.cocotone.jp/"
    COMPANY_EMAIL "info@cocotone.jp"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Cctn
    PLUGIN_CODE Vvte
    PLUGIN_NAME "Voicevox Talk FX"
    FORMATS AU VST3 Standalone
    PRODUCT_NAME ${TARGET_NAME_FX}
    )

file (GLOB_RECURSE ${TARGET_NAME_FX}_source_list CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.hpp
    )

assign_source_group("${${TARGET_NAME_FX}_source_list}")

target_sources(${TARGET_NAME_FX}
    PRIVATE
        ${${TARGET_NAME_FX}_source_list}
    )

target_compile_definitions(${TARGET_NAME_FX}
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(${TARGET_NAME_FX}
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
        voicevox::voicevox_core
        voicevox::voicevox_juce
        cocotone::voicevox_juce_extra
        cocotone::cocotone_song_editor_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Due to windows dll search behaviour.
# VST3 plugin should delay load linked dlls.
# https://docs.analytica.com/index.php/Ana:Embedded_Python/Research_notes
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_link_options(${TARGET_NAME_FX} 
        PUBLIC
            /DELAYLOAD:voicevox_core.dll
    )
endif()

# Pre link and post build command process for Standalone...
if(TARGET ${TARGET_NAME_FX}_Standalone)
    add_custom_command(
        TARGET ${TARGET_NAME_FX}_Standalone
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::voicevox_core> $<TARGET_FILE_DIR:${TARGET_NAME_FX}_Standalone>
    )

    add_custom_command(
        TARGET ${TARGET_NAME_FX}_Standalone
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::onnxruntime> $<TARGET_FILE_DIR:${TARGET_NAME_FX}_Standalone>
    )

    if(TARGET voicevox::onnxruntime_providers_shared)
        add_custom_command(
            TARGET ${TARGET_NAME_FX}_Standalone
            PRE_LINK
            COMMAND
                ${CMAKE_COMMAND} -E
                copy $<TARGET_FILE:voicevox::onnxruntime_providers_shared> $<TARGET_FILE_DIR:${TARGET_NAME_FX}_Standalone>
        )
    endif()

    # Copy resource directory to build directory.
    add_custom_command(
        TARGET ${TARGET_NAME_FX}_Standalone
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_model_dir> $<TARGET_FILE_DIR:${TARGET_NAME_FX}_Standalone>/model
    )

    add_custom_command(
        TARGET ${TARGET_NAME_FX}_Standalone
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_open_jtalk_dic_dir> $<TARGET_FILE_DIR:${TARGET_NAME_FX}_Standalone>/open_jtalk_dic_utf_8
    )
endif()

# Pre link and post build command process for VST3...
if(TARGET ${TARGET_NAME_FX}_VST3)
    add_custom_command(
        TARGET ${TARGET_NAME_FX}_VST3
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::voicevox_core> $<TARGET_FILE_DIR:${TARGET_NAME_FX}_VST3>
    )

    add_custom_command(
        TARGET ${TARGET_NAME_FX}_VST3
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::onnxruntime> $<TARGET_FILE_DIR:${TARGET_NAME_FX}_VST3>
    )

    if(TARGET voicevox::onnxruntime_providers_shared)
        add_custom_command(
            TARGET ${TARGET_NAME_FX}_VST3
            PRE_LINK
            COMMAND
                ${CMAKE_COMMAND} -E
                copy $<TARGET_FILE:voicevox::onnxruntime_providers_shared> $<TARGET_FILE_DIR:${TARGET_NAME_FX}_VST3>
        )
    endif()

    # Copy resource directory to build directory.
    add_custom_command(
        TARGET ${TARGET_NAME_FX}_VST3
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_model_dir> $<TARGET_FILE_DIR:${TARGET_NAME_FX}_VST3>/model
    )

    add_custom_command(
        TARGET ${TARGET_NAME_FX}_VST3
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_open_jtalk_dic_dir> $<TARGET_FILE_DIR:${TARGET_NAME_FX}_VST3>/open_jtalk_dic_utf_8
    )
endif()

# Pre link and post build command process for VST3...
if(TARGET ${TARGET_NAME_FX}_AU)
    add_custom_command(
        TARGET ${TARGET_NAME_FX}_AU
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::voicevox_core> $<TARGET_FILE_DIR:${TARGET_NAME_FX}_AU>
    )

    add_custom_command(
        TARGET ${TARGET_NAME_FX}_AU
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::onnxruntime> $<TARGET_FILE_DIR:${TARGET_NAME_FX}_AU>
    )

    if(TARGET voicevox::onnxruntime_providers_shared)
        add_custom_command(
            TARGET ${TARGET_NAME_FX}_AU
            PRE_LINK
            COMMAND
                ${CMAKE_COMMAND} -E
                copy $<TARGET_FILE:voicevox::onnxruntime_providers_shared> $<TARGET_FILE_DIR:${TARGET_NAME_FX}_AU>
        )
    endif()

    # Copy resource directory to build directory.
    add_custom_command(
        TARGET ${TARGET_NAME_FX}_AU
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_model_dir> $<TARGET_FILE_DIR:${TARGET_NAME_FX}_AU>/model
    )

    add_custom_command(
        TARGET ${TARGET_NAME_FX}_AU
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_open_jtalk_dic_dir> $<TARGET_FILE_DIR:${TARGET_NAME_FX}_AU>/open_jtalk_dic_utf_8
    )
endif()