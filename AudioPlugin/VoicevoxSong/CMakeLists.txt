cmake_minimum_required(VERSION 3.22)

set(PLUGIN_NAME VoicevoxSong)

juce_add_plugin(${PLUGIN_NAME}
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
    PLUGIN_CODE Cvvs
    FORMATS AU VST3 Standalone
    PRODUCT_NAME ${PLUGIN_NAME}
    )

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

file (GLOB_RECURSE ${PLUGIN_NAME}_source_list CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.hpp
    )

assign_source_group("${${PLUGIN_NAME}_source_list}")

target_sources(${PLUGIN_NAME}
    PRIVATE
        ${${PLUGIN_NAME}_source_list}
    )

target_compile_definitions(${PLUGIN_NAME}
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(${PLUGIN_NAME}
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
        voicevox::voicevox_core
        voicevox::voicevox_juce
        cocotone::voicevox_juce_extra
        cocotone::cocotone_song_editor_basics
        cocotone::cocotone_song_editor_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Due to windows dll search behaviour.
# VST3 plugin should delay load linked dlls.
# https://docs.analytica.com/index.php/Ana:Embedded_Python/Research_notes
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_link_options(${PLUGIN_NAME} 
        PUBLIC
            /DELAYLOAD:voicevox_core.dll
    )
endif()

# Pre link and post build command process for Standalone...
if(TARGET ${PLUGIN_NAME}_Standalone)
    add_custom_command(
        TARGET ${PLUGIN_NAME}_Standalone
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::voicevox_core> $<TARGET_FILE_DIR:${PLUGIN_NAME}_Standalone>
    )

    add_custom_command(
        TARGET ${PLUGIN_NAME}_Standalone
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::onnxruntime> $<TARGET_FILE_DIR:${PLUGIN_NAME}_Standalone>
    )

    if(TARGET voicevox::onnxruntime_providers_shared)
        add_custom_command(
            TARGET ${PLUGIN_NAME}_Standalone
            PRE_LINK
            COMMAND
                ${CMAKE_COMMAND} -E
                copy $<TARGET_FILE:voicevox::onnxruntime_providers_shared> $<TARGET_FILE_DIR:${PLUGIN_NAME}_Standalone>
        )
    endif()

    # Copy resource directory to build directory.
    add_custom_command(
        TARGET ${PLUGIN_NAME}_Standalone
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_model_dir> $<TARGET_FILE_DIR:${PLUGIN_NAME}_Standalone>/model
    )

    add_custom_command(
        TARGET ${PLUGIN_NAME}_Standalone
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_open_jtalk_dic_dir> $<TARGET_FILE_DIR:${PLUGIN_NAME}_Standalone>/open_jtalk_dic_utf_8
    )
endif()

# Pre link and post build command process for VST3...
if(TARGET ${PLUGIN_NAME}_VST3)
    add_custom_command(
        TARGET ${PLUGIN_NAME}_VST3
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::voicevox_core> $<TARGET_FILE_DIR:${PLUGIN_NAME}_VST3>
    )

    add_custom_command(
        TARGET ${PLUGIN_NAME}_VST3
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::onnxruntime> $<TARGET_FILE_DIR:${PLUGIN_NAME}_VST3>
    )

    if(TARGET voicevox::onnxruntime_providers_shared)
        add_custom_command(
            TARGET ${PLUGIN_NAME}_VST3
            PRE_LINK
            COMMAND
                ${CMAKE_COMMAND} -E
                copy $<TARGET_FILE:voicevox::onnxruntime_providers_shared> $<TARGET_FILE_DIR:${PLUGIN_NAME}_VST3>
        )
    endif()

    # Copy resource directory to build directory.
    add_custom_command(
        TARGET ${PLUGIN_NAME}_VST3
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_model_dir> $<TARGET_FILE_DIR:${PLUGIN_NAME}_VST3>/model
    )

    add_custom_command(
        TARGET ${PLUGIN_NAME}_VST3
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_open_jtalk_dic_dir> $<TARGET_FILE_DIR:${PLUGIN_NAME}_VST3>/open_jtalk_dic_utf_8
    )
endif()

# Pre link and post build command process for VST3...
if(TARGET ${PLUGIN_NAME}_AU)
    add_custom_command(
        TARGET ${PLUGIN_NAME}_AU
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::voicevox_core> $<TARGET_FILE_DIR:${PLUGIN_NAME}_AU>
    )

    add_custom_command(
        TARGET ${PLUGIN_NAME}_AU
        PRE_LINK
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:voicevox::onnxruntime> $<TARGET_FILE_DIR:${PLUGIN_NAME}_AU>
    )

    if(TARGET voicevox::onnxruntime_providers_shared)
        add_custom_command(
            TARGET ${PLUGIN_NAME}_AU
            PRE_LINK
            COMMAND
                ${CMAKE_COMMAND} -E
                copy $<TARGET_FILE:voicevox::onnxruntime_providers_shared> $<TARGET_FILE_DIR:${PLUGIN_NAME}_AU>
        )
    endif()

    # Copy resource directory to build directory.
    add_custom_command(
        TARGET ${PLUGIN_NAME}_AU
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_model_dir> $<TARGET_FILE_DIR:${PLUGIN_NAME}_AU>/model
    )

    add_custom_command(
        TARGET ${PLUGIN_NAME}_AU
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy_directory $<TARGET_PROPERTY:voicevox::voicevox_resource,resource_open_jtalk_dic_dir> $<TARGET_FILE_DIR:${PLUGIN_NAME}_AU>/open_jtalk_dic_utf_8
    )
endif()