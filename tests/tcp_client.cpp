#include "../include/nets.hpp"

#include "mynet.hpp"

#include <print>
#include <iostream>

class Client : public nets::TcpClient<MessageIds, Remote>
{
    public:
        using TcpClient<MessageIds, Remote>::TcpClient;

        virtual void onConnection(std::shared_ptr<Remote> server) override
        {
            std::println("Connected to server!");

            std::println("{}", server->getPort());

            while(server->isConnected())
            {
                std::this_thread::sleep_for(PingTime{2.25});
            }

            std::println("Server closed connection...");
        }
};

int main()
{
    Client client {
        "localhost",
        "60000",
        Remote::PingTime{4},
        Remote::PingTime{6}
    };

    client.connect();

    while(true)
    {
    }

    return 0;
}