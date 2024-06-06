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

            server->onFailedReading = [&, this](std::optional<boost::system::error_code> error){
                if(error.has_value())
                {
                    std::println("Error: {}.", error.value().message());
                }
                else 
                {
                    std::println("Error.");
                }
            };

            server->setOnReceiving(
                MessageIds::message_response,
                [&, this](Collection message, nets::TcpRemote<MessageIds>& server)
                {
                    std::println("Received message from server: \"{}\".", message.retrieve<std::string>());
                }
            );

            while(server->isConnected())
            {
                std::string message_to_be_sent;

                std::getline(std::cin, message_to_be_sent);

                server->send(
                    Collection{} << MessageIds::message_request << message_to_be_sent
                );
            }

            std::println("Server closed connection...");

            disconnect();
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