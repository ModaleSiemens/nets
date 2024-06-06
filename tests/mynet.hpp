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
};