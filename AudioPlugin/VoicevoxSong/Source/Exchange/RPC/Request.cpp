#include "Request.h"

namespace cctn
{
namespace song
{
namespace exchange
{

juce::var postData(const juce::String& url, const juce::String& data)
{
    juce::URL apiUrl = juce::URL(url).withPOSTData(data);

    juce::String headers;
    headers << "Content-Type: application/json";

    std::unique_ptr<juce::InputStream> stream =
        apiUrl.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
            .withHttpRequestCmd("POST")
            .withExtraHeaders(headers)
            .withConnectionTimeoutMs(1800000) // 3 minutes
        );

    if (stream != nullptr)
    {
        juce::String response = stream->readEntireStreamAsString();
        DBG(response);

        const auto var = juce::JSON::parse(response);

        return var;
    }
    else
    {
        DBG("Error: Could not open stream");
    }

    return juce::var("");
}

}
}
}
