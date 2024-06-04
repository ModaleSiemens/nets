#pragma once

#include <print>

enum class MessageIds
{
    ping_request, ping_response,
    message_request,
    message_response
};

class Remote : public nets::TcpRemote<MessageIds>
{
    public:
        using TcpRemote<MessageIds>::TcpRemote;

        void onFailedSending(mdsm::Collection message) override
        {
            std::println("Failed to send message...");
        }

        void onFailedReading(std::optional<boost::system::error_code> error) override
        {
            std::println("Failed to read from remote...");
        }

        void onPingingTimeout() override
        {
            std::println("Remote didn't respond in time...");
        }
};