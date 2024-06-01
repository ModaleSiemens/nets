#include "../include/nets.hpp"

#include <print>

class Server : public nets::TcpServer
{
    public:
        using TcpServer::TcpServer;

        virtual void onClientConnection(nets::TcpRemote& client) override
        {
            std::println("Client connected!");

            closeConnection(client);

            std::println("Closed connection...");
        }

        virtual void onForbiddenClientConnection(nets::TcpRemote& client) override
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