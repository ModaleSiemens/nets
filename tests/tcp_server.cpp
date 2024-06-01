#include "../include/nets.hpp"

#include "mynet.hpp"

#include <print>

class Server : public nets::TcpServer<MessageIds, Remote>
{
    public:
        using TcpServer<MessageIds, Remote>::TcpServer;

        virtual void onClientConnection(Remote& client) override
        {
            std::println("Client connected!");

            while(true);

            //std::println("{}", closeConnection(client));

            //std::println("Closed connection...");
        }

        virtual void onForbiddenClientConnection(Remote& client) override
        {
            std::println("Client connected while server wasn't accepting requests...");

            closeConnection(client);

            std::println("Closed connection...");
        }
};

int main()
{
    Server server {
        60'000,
        nets::IPVersion::ipv4
    };

    server.startAccepting();

    while(true)
    {
    }

    return 0;
}