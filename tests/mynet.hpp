#pragma once

#include <print>

enum class MessageIds
{
    ping_request, ping_response, probe
};

class Remote : public nets::TcpRemote<MessageIds>
{
    public:
        using TcpRemote<MessageIds>::TcpRemote;

        void onFailedSending(const mdsm::Collection& message) override
        {
            std::println("Failed to send message...");
        }

        void onPingingTimeout() override
        {
            std::println("Remote didn't respond in time...");
        }

        void onPingFailedSending() override
        {
            std::println("Failed to send ping...");
        }
};