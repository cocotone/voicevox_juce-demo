#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
    , editorState("editor_sate")
    , applicationState("application_state")
    , isSyncToHostTransport(false)
{
    // Application state related.
    applicationState.setProperty("Player_CanPlay", juce::var(false), nullptr);
    applicationState.setProperty("Player_IsPlaying", juce::var(false), nullptr);
    applicationState.setProperty("Player_IsLooping", juce::var(false), nullptr);
    applicationState.setProperty("Player_IsSyncToHostTransport", juce::var(false), nullptr);

    applicationState.addListener(this);

    editorState.setProperty("VoicevoxEngine_IsTaskRunning", juce::var(false), nullptr);
    editorState.setProperty("VoicevoxEngine_SelectedTalkSpeakerIdentifier", juce::var(""), nullptr);
    editorState.setProperty("VoicevoxEngine_SelectedHummingSpeakerIdentifier", juce::var(""), nullptr);
    editorState.setProperty("VoicevoxEngine_HasSpeakerListUpdated", juce::var(false), nullptr);

    voicevoxMapSpeakerIdentifierToSpeakerId.clear();
    voicevoxTalkSpeakerIdentifierList.clear();
    voicevoxHummingSpeakerIdentifierList.clear();

    // Audio file player related.
    audioFormatManager = std::make_unique<juce::AudioFormatManager>();
    audioFormatManager->registerBasicFormats();

    audioBufferingThread = std::make_unique<juce::TimeSliceThread>("AudioBufferingThread");
    audioBufferingThread->startThread();

    audioTransportSource = std::make_unique<juce::AudioTransportSource>();

    audioThumbnail = std::make_unique<juce::AudioThumbnail>(512, *audioFormatManager.get(), audioThumbnailCache);
   
    voicevoxEngine = std::make_unique<cctn::VoicevoxEngine>();

    audioTransportSource->addChangeListener(this);
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    voicevoxEngine->shutdown();

    audioTransportSource->removeChangeListener(this);

    applicationState.removeListener(this);
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (sampleRate, samplesPerBlock);

    audioTransportSource->prepareToPlay(samplesPerBlock, sampleRate);

    voicevoxEngine->start();
    voicevoxMapSpeakerIdentifierToSpeakerId = voicevoxEngine->getSpeakerIdentifierToSpeakerIdMap();
    voicevoxTalkSpeakerIdentifierList = voicevoxEngine->getTalkSpeakerIdentifierList();
    voicevoxHummingSpeakerIdentifierList = voicevoxEngine->getHummingSpeakerIdentifierList();

    editorState.setProperty("VoicevoxEngine_HasSpeakerIdUpdated", juce::var(true), nullptr);
    
    juce::Logger::outputDebugString(this->getMetaJsonStringify());

    if (hostSyncAudioSouce.get() != nullptr)
    {
        hostSyncAudioSouce->resamplerForChannelL->reset();
        hostSyncAudioSouce->resamplerForChannelR->reset();
    }
}

void AudioPluginAudioProcessor::releaseResources()
{
    voicevoxEngine->stop();
    voicevoxTalkSpeakerIdentifierList.clear();
    voicevoxHummingSpeakerIdentifierList.clear();
    voicevoxMapSpeakerIdentifierToSpeakerId.clear();
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    const auto current_host_poisition_info =
        [&] 
        {
            if (const auto* play_head = getPlayHead())
            {
                if (const auto result = play_head->getPosition())
                {
                    return result.orFallback(juce::AudioPlayHead::PositionInfo{});
                }
            }

            // If the host fails to provide the current time, we'll just use default values
            return juce::AudioPlayHead::PositionInfo{};
        }();

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // SECTION: Clear unused bus audio buffer.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }

    // SECTION: Audio rendering.
    if (isSyncToHostTransport)
    {
        // SECTION: Synchronization to plugin host
        if (hostSyncAudioSouce.get() != nullptr)
        {
            juce::AudioBuffer<float> audio_buffer_resampler_retriver;
            audio_buffer_resampler_retriver.makeCopyOf(buffer);

            if (current_host_poisition_info.getIsPlaying())
            {
                const double position_seconds_in_host = current_host_poisition_info.getTimeInSeconds().orFallback(0.0);
                const double sample_rate_audio_source = hostSyncAudioSouce->sampleRateAudioSource;
                const juce::int64 next_read_position = position_seconds_in_host * sample_rate_audio_source;
                hostSyncAudioSouce->memoryAudioSource->setNextReadPosition(next_read_position);

                const double sr_ratio_from_source_to_device = sample_rate_audio_source / this->getSampleRate();
                const int num_samples_to_device = buffer.getNumSamples();
                const int num_samples_from_source = num_samples_to_device * sr_ratio_from_source_to_device;

                juce::AudioBuffer<float> audio_buffer_source_retriver;
                audio_buffer_source_retriver.setSize(2, num_samples_from_source);
                audio_buffer_source_retriver.clear();

                juce::AudioSourceChannelInfo audio_source_retriever(audio_buffer_source_retriver);
                hostSyncAudioSouce->memoryAudioSource->getNextAudioBlock(audio_source_retriever);

                hostSyncAudioSouce->resamplerForChannelL->process(
                    sr_ratio_from_source_to_device,
                    audio_source_retriever.buffer->getReadPointer(0),
                    audio_buffer_resampler_retriver.getWritePointer(0),
                    audio_buffer_resampler_retriver.getNumSamples(),
                    audio_source_retriever.buffer->getNumSamples(),
                    64);

                hostSyncAudioSouce->resamplerForChannelR->process(
                    sr_ratio_from_source_to_device,
                    audio_source_retriever.buffer->getReadPointer(1),
                    audio_buffer_resampler_retriver.getWritePointer(1),
                    audio_buffer_resampler_retriver.getNumSamples(),
                    audio_source_retriever.buffer->getNumSamples(),
                    64);
            }

            buffer.copyFrom(0, 0, audio_buffer_resampler_retriver.getReadPointer(0), audio_buffer_resampler_retriver.getNumSamples());
            buffer.copyFrom(1, 0, audio_buffer_resampler_retriver.getReadPointer(1), audio_buffer_resampler_retriver.getNumSamples());
        }
    }
    else
    {
        // SECTION: Not synchronization to plugin host
        for (const juce::MidiMessageMetadata metadata : midiMessages)
        {
            const auto message = metadata.getMessage();
            if (message.isNoteOn() && message.getNoteNumber() == 60)
            {
                audioTransportSource->start();
            }
        }

        juce::AudioSourceChannelInfo buffer_info(buffer);
        audioTransportSource->getNextAudioBlock(buffer_info);
    }

    // Now ask the host for the current time so we can store it to be displayed later...
    updateCurrentTimeInfoFromHost();
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
void AudioPluginAudioProcessor::loadAudioFile(const juce::File& fileToLoad)
{
    // Unload the previous file source and delete it..
    audioThumbnail->clear();
    audioTransportSource->stop();
    audioTransportSource->setSource(nullptr);
    audioFormatReaderSource.reset();

    juce::AudioFormatReader* reader = audioFormatManager->createReaderFor(fileToLoad);

    if (reader != nullptr)
    {
        audioFormatReaderSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);

        audioTransportSource->setSource(audioFormatReaderSource.get(),
            32768,
            audioBufferingThread.get(),
            reader->sampleRate,
            2);

        // Update audio thumbnail
        audioBufferForThumbnail.clear();
        audioBufferForThumbnail.setSize(2, reader->lengthInSamples);
        reader->read(&audioBufferForThumbnail, 0, reader->lengthInSamples, 0, true, true);

        juce::Uuid uuid;
        audioThumbnail->setSource(&audioBufferForThumbnail, reader->sampleRate, uuid.hash());

        // Update can play or not.
        if (audioTransportSource->getTotalLength() > 0)
        {
            applicationState.setProperty("Player_CanPlay", juce::var(true), nullptr);
        }
        else
        {
            applicationState.setProperty("Player_CanPlay", juce::var(false), nullptr);
        }
    }
}

void AudioPluginAudioProcessor::loadAudioFileStream(std::unique_ptr<juce::InputStream> audioFileStream)
{
    // Unload the previous file source and delete it..
    audioThumbnail->clear();
    audioTransportSource->stop();
    audioTransportSource->setSource(nullptr);
    audioFormatReaderSource.reset();

    juce::AudioFormatReader* reader = audioFormatManager->createReaderFor(std::move(audioFileStream));

    if (reader != nullptr)
    {
        audioFormatReaderSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);

        audioTransportSource->setSource(audioFormatReaderSource.get(),
            32768,
            audioBufferingThread.get(),
            reader->sampleRate,
            2);

        // Update audio thumbnail
        audioBufferForThumbnail.clear();
        audioBufferForThumbnail.setSize(2, reader->lengthInSamples);
        reader->read(&audioBufferForThumbnail, 0, reader->lengthInSamples, 0, true, true);

        juce::Uuid uuid;
        audioThumbnail->setSource(&audioBufferForThumbnail, reader->sampleRate, uuid.hash());

        // Update can play or not.
        if (audioTransportSource->getTotalLength() > 0)
        {
            applicationState.setProperty("Player_CanPlay", juce::var(true), nullptr);
        }
        else
        {
            applicationState.setProperty("Player_CanPlay", juce::var(false), nullptr);
        }
    }
}

void AudioPluginAudioProcessor::loadVoicevoxEngineAudioBufferInfo(const cctn::AudioBufferInfo& audioBufferInfo)
{
    // Unload the previous file source and delete it..
    audioThumbnail->clear();
    audioTransportSource->stop();
    audioTransportSource->setSource(nullptr);
    memoryAudioSource.reset();

    juce::AudioBuffer<float> stereonized_buffer;
    stereonized_buffer.setSize(2, audioBufferInfo.audioBuffer.getNumSamples());
    stereonized_buffer.copyFrom(0, 0, audioBufferInfo.audioBuffer.getReadPointer(0), audioBufferInfo.audioBuffer.getNumSamples());
    stereonized_buffer.copyFrom(1, 0, audioBufferInfo.audioBuffer.getReadPointer(0), audioBufferInfo.audioBuffer.getNumSamples());

    memoryAudioSource = std::make_unique<juce::MemoryAudioSource>(stereonized_buffer, true, false);

    {
        hostSyncAudioSouce = std::make_unique<HostSyncAudioSource>();
        hostSyncAudioSouce->memoryAudioSource = std::make_unique<juce::MemoryAudioSource>(stereonized_buffer, true, false);
        hostSyncAudioSouce->sampleRateAudioSource = audioBufferInfo.sampleRate;
        hostSyncAudioSouce->resamplerForChannelL = std::make_unique<juce::LagrangeInterpolator>();
        hostSyncAudioSouce->resamplerForChannelL->reset();
        hostSyncAudioSouce->resamplerForChannelR = std::make_unique<juce::LagrangeInterpolator>();
        hostSyncAudioSouce->resamplerForChannelR->reset();
    }

    audioTransportSource->setSource(memoryAudioSource.get(),
        32768,
        audioBufferingThread.get(),
        audioBufferInfo.sampleRate,
        2);

    // Update audio thumbnail
    audioBufferForThumbnail.clear();
    audioBufferForThumbnail.makeCopyOf(stereonized_buffer, false);

    juce::Uuid uuid;
    audioThumbnail->setSource(&audioBufferForThumbnail, audioBufferInfo.sampleRate, uuid.hash());

    // Update can play or not.
    if (audioTransportSource->getTotalLength() > 0)
    {
        applicationState.setProperty("Player_CanPlay", juce::var(true), nullptr);
    }
    else
    {
        applicationState.setProperty("Player_CanPlay", juce::var(false), nullptr);
    }
}

void AudioPluginAudioProcessor::clearAudioFileHandle()
{
    // Unload the previous file source and delete it..
    audioThumbnail->clear();
    audioTransportSource->stop();
    audioTransportSource->setSource(nullptr);
    audioFormatReaderSource.reset();

    applicationState.setProperty("Player_CanPlay", juce::var(false), nullptr);
}

//==============================================================================
void AudioPluginAudioProcessor::requestSynthesis(juce::int64 speakerId, const juce::String& audio_query_json)
{
    //const auto result = voicevoxClient->loadModel(speakerId);
    //if (result.failed())
    //{
    //    juce::Logger::outputDebugString(result.getErrorMessage());
    //}

    //juce::var json_parsed = juce::JSON::parse(audio_query_json);
    //const auto audio_query_json_conv = juce::JSON::toString(json_parsed, true);

    //auto memory_wav = voicevoxClient->synthesis(speakerId, audio_query_json_conv);

    //if (memory_wav.has_value())
    //{
    //    loadAudioFileStream(std::make_unique<juce::MemoryInputStream>(memory_wav.value(), true));
    //}
}

void AudioPluginAudioProcessor::requestTextToSpeech(juce::int64 speakerId, const juce::String& text)
{
    cctn::VoicevoxEngineRequest request;
    request.requestId = juce::Uuid();
    request.speakerId = voicevoxMapSpeakerIdentifierToSpeakerId[editorState.getProperty("VoicevoxEngine_SelectedTalkSpeakerIdentifier").toString()];
    request.text = text;
    request.processType = cctn::VoicevoxEngineProcessType::kTalk;

    voicevoxEngine->requestAsync(request,
        [this](const cctn::VoicevoxEngineArtefact& artefact) {
            juce::Logger::outputDebugString(artefact.requestId.toString());

            if (artefact.wavBinary.has_value())
            {
                juce::MessageManager::callAsync(
                    [this, wav_binary = artefact.wavBinary.value()] {
                        this->loadAudioFileStream(std::make_unique<juce::MemoryInputStream>(wav_binary, true));

                        editorState.setProperty("VoicevoxEngine_IsTaskRunning", juce::var(false), nullptr);
                    });
            }
            else
            {
                juce::MessageManager::callAsync(
                    [this] {
                        this->clearAudioFileHandle();

                        editorState.setProperty("VoicevoxEngine_IsTaskRunning", juce::var(false), nullptr);
                    });
            }
        });

    editorState.setProperty("VoicevoxEngine_IsTaskRunning", juce::var(true), nullptr);
}

void AudioPluginAudioProcessor::requestHumming(juce::int64 speakerId, const juce::String& text)
{
    cctn::VoicevoxEngineRequest request;
    request.requestId = juce::Uuid();
    request.speakerId = voicevoxMapSpeakerIdentifierToSpeakerId[editorState.getProperty("VoicevoxEngine_SelectedHummingSpeakerIdentifier").toString()];
    //request.audioQuery = text;
    request.scoreJson = text;
    request.sampleRate = 24000;
    request.processType = cctn::VoicevoxEngineProcessType::kHumming;

    voicevoxEngine->requestAsync(request,
        [this](const cctn::VoicevoxEngineArtefact& artefact) {
            juce::Logger::outputDebugString(artefact.requestId.toString());

            if (artefact.audioBufferInfo.has_value())
            {
                juce::MessageManager::callAsync(
                    [this, audio_buffer_info = artefact.audioBufferInfo.value()] {
                        this->loadVoicevoxEngineAudioBufferInfo(audio_buffer_info);

                        editorState.setProperty("VoicevoxEngine_IsTaskRunning", juce::var(false), nullptr);
                    });
            }
            else
            {
                juce::MessageManager::callAsync(
                    [this] {
                        this->clearAudioFileHandle();

                        editorState.setProperty("VoicevoxEngine_IsTaskRunning", juce::var(false), nullptr);
                    });
            }
        });

    editorState.setProperty("VoicevoxEngine_IsTaskRunning", juce::var(true), nullptr);
}

juce::String AudioPluginAudioProcessor::getMetaJsonStringify()
{
    return juce::JSON::toString(voicevoxEngine->getMetaJson());
}

double AudioPluginAudioProcessor::getHostSyncAudioSourceLengthInSeconds() const
{
    if (hostSyncAudioSouce.get() != nullptr)
    {
        if (hostSyncAudioSouce->sampleRateAudioSource != 0.0)
        {
            return hostSyncAudioSouce->memoryAudioSource->getTotalLength() / hostSyncAudioSouce->sampleRateAudioSource;
        }
    }

    return 0.0;
}

//==============================================================================
void AudioPluginAudioProcessor::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == audioTransportSource.get())
    {
        // End operation.
        if (audioTransportSource->getTotalLength() > 0 && audioTransportSource->hasStreamFinished())
        {
            audioTransportSource->setNextReadPosition(0);

            // Loop operation.
            if (applicationState.getProperty("Player_IsLooping"))
            {
                audioTransportSource->start();
            }
        }

        // Update playing or not.
        applicationState.setProperty("Player_IsPlaying", audioTransportSource->isPlaying(), nullptr);
    }
}

//==============================================================================
void AudioPluginAudioProcessor::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& propertyId)
{
    if (treeWhosePropertyHasChanged == applicationState)
    {
        if (propertyId.toString() == "Player_IsPlaying")
        {
            if (!isSyncToHostTransport)
            {
                // Scoped listener detachment.
                const bool should_play = (bool)applicationState.getProperty(propertyId);
                if (should_play)
                {
                    audioTransportSource->start();

                    // Set triggered position.
                    playTriggeredPositionInfo = getLastPositionInfo();
                }
                else
                {
                    audioTransportSource->stop();
                }
            }
        }
        else if (propertyId.toString() == "Player_IsSyncToHostTransport")
        {
            isSyncToHostTransport = (bool)applicationState.getProperty(propertyId);
        }
    }
}

//==============================================================================
void AudioPluginAudioProcessor::updateCurrentTimeInfoFromHost()
{
    const auto newInfo = [&]
        {
            if (const auto* play_head = getPlayHead())
            {
                if (const auto result = play_head->getPosition())
                {
                    return result.orFallback(juce::AudioPlayHead::PositionInfo{});
                }
            }

            // If the host fails to provide the current time, we'll just use default values
            return juce::AudioPlayHead::PositionInfo{};
        }();

    lastPositionInfo.set(newInfo);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
