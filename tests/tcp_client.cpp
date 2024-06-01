#include "../include/nets.hpp"

#include <print>

class Client : public nets::TcpClient
{
    public:
        using TcpClient::TcpClient;

        virtual void onConnection(nets::TcpRemote& server) override
        {
            std::println("Connected to server!");

            while(server.connectionIsOpen())
            {

            }

            std::println("Server closed connection...");
        }
};

int main()
{
    Client client {
        "localhost",
        "60000"
    };

    client.connect();

    return 0;
}