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
{
    // Application state related.
    applicationState.setProperty("Player_CanPlay", juce::var(false), nullptr);
    applicationState.setProperty("Player_IsPlaying", juce::var(false), nullptr);
    applicationState.setProperty("Player_IsLooping", juce::var(false), nullptr);

    applicationState.addListener(this);

    editorState.setProperty("VoicevoxEngine_IsTaskRunning", juce::var(false), nullptr);
    editorState.setProperty("VoicevoxEngine_SelectedSpeakerIdentifier", juce::var(""), nullptr);
    editorState.setProperty("VoicevoxEngine_HasSpeakerListUpdated", juce::var(false), nullptr);

    voicevoxMapSpeakerIdentifierToSpeakerId.clear();
    voicevoxSpeakerIdentifierList.clear();

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
    voicevoxSpeakerIdentifierList = voicevoxEngine->getSpeakerIdentifierList();

    editorState.setProperty("VoicevoxEngine_HasSpeakerIdUpdated", juce::var(true), nullptr);
}

void AudioPluginAudioProcessor::releaseResources()
{
    voicevoxEngine->stop();
    voicevoxSpeakerIdentifierList.clear();
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

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        juce::ignoreUnused (channelData);
        // ..do something to the data...
    }

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
    request.speakerId = voicevoxMapSpeakerIdentifierToSpeakerId[editorState.getProperty("VoicevoxEngine_SelectedSpeakerIdentifier").toString()];
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
    request.speakerId = voicevoxMapSpeakerIdentifierToSpeakerId[editorState.getProperty("VoicevoxEngine_SelectedSpeakerIdentifier").toString()];
    request.text = text;
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
            // Scoped listener detachment.
            const bool should_play = (bool)applicationState.getProperty(propertyId);
            if (should_play)
            {
                audioTransportSource->start();
            }
            else
            {
                audioTransportSource->stop();
            }
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
