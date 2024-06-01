#include "../include/nets.hpp"

#include "mynet.hpp"

#include <print>

class Client : public nets::TcpClient<MessageIds, Remote>
{
    public:
        using TcpClient<MessageIds, Remote>::TcpClient;

        virtual void onConnection(std::shared_ptr<Remote> server) override
        {
            std::println("Connected to server!");

            while(server->connectionIsOpen())
            {

            }

            std::println("Server closed connection...");
        }

        virtual void onDisconnection(std::shared_ptr<Remote> server) override
        {
            std::println("Disconnected...");
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