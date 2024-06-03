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
            using namespace mdsm;

            server->setOnReceiving(
                MessageIds::message_response,
                [&, this](Collection message, nets::TcpRemote<MessageIds>& server)
                {
                    std::println("Received message: \"{}\".", message.retrieve<std::string>());
                }
            );

            Collection message;

            while(server->isConnected())
            {
                std::string message_to_be_sent;

                std::cin >> message_to_be_sent;

                message << message_to_be_sent;

                server->send(message);
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