#include "../include/nets.hpp"

#include "mynet.hpp"

#include <print>

class Client : public nets::TcpClient<MessageIds>
{
    public:
        using TcpClient::TcpClient;

        virtual void onConnection(nets::TcpRemote<MessageIds>& server) override
        {
            std::println("Connected to server!");

            while(server.connectionIsOpen())
            {

            }

            std::println("Server closed connection...");
        }

        virtual void onDisconnection(nets::TcpRemote<MessageIds>& server) override
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