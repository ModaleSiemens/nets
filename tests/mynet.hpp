#pragma once

#include <print>

enum class MessageIds
{
    ping_request, ping_response, probe
};

class Remote : public nets::TcpRemote<MessageIds>
{
    public:
        Remote(
            boost::asio::io_context& io_context,
            const PingTime ping_timeout_period,
            const PingTime ping_delay
        )
        :
            TcpRemote(io_context, ping_timeout_period, ping_delay)
        {
            enableBeingPinged();
            enablePinging();
            enableReceivingMessages();
        }

        void onFailedSending(const mdsm::Collection& message) override
        {
            std::println("Failed to send message...");
        }

        void onPingingTimeout() override
        {
            std::println("Server didn't respond in time...");
        }

        void onPingFailedSending() override
        {
            std::println("Failed to send ping...");
        }
};