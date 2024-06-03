#include "../include/nets.hpp"

#include "mynet.hpp"

#include <print>

class Server : public nets::TcpServer<MessageIds, Remote>
{
    public:
        using TcpServer<MessageIds, Remote>::TcpServer;

        virtual void onClientConnection(std::shared_ptr<Remote> client) override
        {
            std::println("Client connected!");

            std::println("{}", client->getPort());

            while(client->isConnected())
            {
                //std::println("{}", client->isConnected());
            }

            std::println("Client disconnected...");

            closeConnection(client);
        }

        virtual void onForbiddenClientConnection(std::shared_ptr<Remote> client) override
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
        nets::IPVersion::ipv4,
        Remote::PingTime{4},
        Remote::PingTime{6} 
    };

    server.startAccepting();

    while(true)
    {
    }

    return 0;
}