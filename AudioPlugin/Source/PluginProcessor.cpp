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

    hostSyncAudioSourcePlayer = std::make_unique<cctn::HostSyncAudioSourcePlayer>();

    audioThumbnail = std::make_unique<juce::AudioThumbnail>(512, *audioFormatManager.get(), audioThumbnailCache);
    audioDataForAudioThumbnail = std::make_unique<AudioDataForAudioThumbnail>();
    audioDataForAudioThumbnail->audioBuffer.clear();
   
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

    hostSyncAudioSourcePlayer->prepareToPlay(samplesPerBlock, sampleRate);
}

void AudioPluginAudioProcessor::releaseResources()
{
    voicevoxEngine->stop();
    voicevoxTalkSpeakerIdentifierList.clear();
    voicevoxHummingSpeakerIdentifierList.clear();
    voicevoxMapSpeakerIdentifierToSpeakerId.clear();

    hostSyncAudioSourcePlayer->releaseResources();
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

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& audioBuffer,
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
        audioBuffer.clear (i, 0, audioBuffer.getNumSamples());
    }

    // SECTION: Audio rendering.
    if (isSyncToHostTransport)
    {
        hostSyncAudioSourcePlayer->processBlockWithPositionInfo(audioBuffer, midiMessages, current_host_poisition_info);
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

        juce::AudioSourceChannelInfo buffer_info(audioBuffer);
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
        audioDataForAudioThumbnail->sampleRate = reader->sampleRate;
        audioDataForAudioThumbnail->audioBuffer.clear();
        audioDataForAudioThumbnail->audioBuffer.setSize(2, reader->lengthInSamples);
        reader->read(&audioDataForAudioThumbnail->audioBuffer, 0, reader->lengthInSamples, 0, true, true);
        
        juce::MessageManager::callAsync(
            [this] {
                this->resetAudioThumbnail();
            });

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
        audioDataForAudioThumbnail->sampleRate = reader->sampleRate;
        audioDataForAudioThumbnail->audioBuffer.clear();
        audioDataForAudioThumbnail->audioBuffer.setSize(2, reader->lengthInSamples);
        reader->read(&audioDataForAudioThumbnail->audioBuffer, 0, reader->lengthInSamples, 0, true, true);

        juce::MessageManager::callAsync(
            [this] {
                this->resetAudioThumbnail();
                this->updatePlayerState();
            });
    }
}

void AudioPluginAudioProcessor::loadVoicevoxEngineAudioBufferInfo(const cctn::AudioBufferInfo& audioBufferInfo)
{
    // Unload the previous file source and delete it..
    audioTransportSource->stop();
    audioTransportSource->setSource(nullptr);
    memoryAudioSource.reset();

    juce::AudioBuffer<float> stereonized_buffer;
    stereonized_buffer.setSize(2, audioBufferInfo.audioBuffer.getNumSamples());
    stereonized_buffer.copyFrom(0, 0, audioBufferInfo.audioBuffer.getReadPointer(0), audioBufferInfo.audioBuffer.getNumSamples());
    stereonized_buffer.copyFrom(1, 0, audioBufferInfo.audioBuffer.getReadPointer(0), audioBufferInfo.audioBuffer.getNumSamples());

    memoryAudioSource = std::make_unique<juce::MemoryAudioSource>(stereonized_buffer, true, false);

    hostSyncAudioSourcePlayer->setAudioBufferToPlay(stereonized_buffer, audioBufferInfo.sampleRate);

    audioTransportSource->setSource(memoryAudioSource.get(),
        32768,
        audioBufferingThread.get(),
        audioBufferInfo.sampleRate,
        2);

    // Update audio thumbnail
    audioDataForAudioThumbnail->sampleRate = audioBufferInfo.sampleRate;
    audioDataForAudioThumbnail->audioBuffer.clear();
    audioDataForAudioThumbnail->audioBuffer.makeCopyOf(stereonized_buffer, false);

    juce::MessageManager::callAsync(
        [this] {
            this->resetAudioThumbnail();
            this->updatePlayerState();
        });
}

void AudioPluginAudioProcessor::clearAudioFileHandle()
{
    // Unload the previous file source and delete it..
    audioTransportSource->stop();
    audioTransportSource->setSource(nullptr);
    audioFormatReaderSource.reset();

    // Update audio thumbnail
    audioDataForAudioThumbnail->sampleRate = 0.0;
    audioDataForAudioThumbnail->audioBuffer.clear();

    juce::MessageManager::callAsync(
        [this] {
            this->resetAudioThumbnail();
            this->updatePlayerState();
        });
}

void AudioPluginAudioProcessor::resetAudioThumbnail()
{
    audioThumbnail->clear();

    juce::Uuid uuid;
    audioThumbnail->setSource(&audioDataForAudioThumbnail->audioBuffer, audioDataForAudioThumbnail->sampleRate, uuid.hash());
}

void AudioPluginAudioProcessor::updatePlayerState()
{
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
                this->loadAudioFileStream(std::make_unique<juce::MemoryInputStream>(artefact.wavBinary.value(), true));

                juce::MessageManager::callAsync(
                    [this] {
                        editorState.setProperty("VoicevoxEngine_IsTaskRunning", juce::var(false), nullptr);
                    });
            }
            else
            {
                this->clearAudioFileHandle();

                juce::MessageManager::callAsync(
                    [this] {
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
                this->loadVoicevoxEngineAudioBufferInfo(artefact.audioBufferInfo.value());

                juce::MessageManager::callAsync(
                    [this] {
                        editorState.setProperty("VoicevoxEngine_IsTaskRunning", juce::var(false), nullptr);
                    });
            }
            else
            {
                this->clearAudioFileHandle();
                
                juce::MessageManager::callAsync(
                    [this] {
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

//==============================================================================
double AudioPluginAudioProcessor::getHostSyncAudioSourceLengthInSeconds() const
{
    return hostSyncAudioSourcePlayer->getTimeLengthInSeconds();
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
