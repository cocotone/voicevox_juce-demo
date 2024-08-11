#pragma once

#include "Request.h"

namespace cctn
{
namespace song
{
namespace exchange
{

class ExchangeProcessingTask :
    public juce::ThreadWithProgressWindow
{
public:
    ExchangeProcessingTask(std::function<void(const juce::File& inputFile, const juce::File& outputFile)> functionToCall, const juce::File& fileToLoad)
        : juce::ThreadWithProgressWindow("processing...", true, false)
        , fileToProcess(fileToLoad)
        , fileFromResult("")
        , callbackFunction(functionToCall)
    {
        setProgress(-1.0);
    }

    void run()
    {
        juce::DynamicObject::Ptr dynamic_obj = new juce::DynamicObject();
        dynamic_obj->setProperty("inputFilePath", fileToProcess.getFullPathName());
        dynamic_obj->setProperty("outputDirectory", fileToProcess.getSiblingFile(".exchange").getFullPathName());
        const auto json = juce::JSON::toString(dynamic_obj.get());
        const auto json_return_value = cctn::song::exchange::postData("http://localhost:52051", json);

        if (json_return_value.hasProperty("outputFilePath"))
        {
            fileFromResult = juce::File(json_return_value.getProperty("outputFilePath", ""));
        }
        else
        {
            juce::Logger::outputDebugString("[ExchangeProcessingTask] has no value.");
        }
    }

    virtual void threadComplete(bool userPressedCancel) override
    {
        callbackFunction(fileToProcess, fileFromResult);
    }

private:
    juce::File fileToProcess;
    juce::File fileFromResult;
    std::function<void(const juce::File&, const juce::File&)> callbackFunction;
};

}
}
}
