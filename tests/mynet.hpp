#pragma once

enum class MessageIds
{
    ping_request, ping_response, probe
};

class Remote : public nets::TcpRemote<MessageIds>
{
    void initialize() override
    {
        startPinging();
    }
};