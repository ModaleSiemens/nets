#include "../include/nets.hpp"

#include "mynet.hpp"

#include <print>

class Server : public nets::TcpServer<MessageIds, Remote>
{
    public:
        using TcpServer<MessageIds, Remote>::TcpServer;

        virtual void onClientConnection(std::shared_ptr<Remote> client) override
        {
            using namespace mdsm;

            client->setOnReceiving(
                MessageIds::message_request,
                [&, this](Collection message, nets::TcpRemote<MessageIds>& server)
                {
                    const std::string received_string {message.retrieve<std::string>()};

                    std::println("Received message from client: \"{}\".", received_string);

                    client->send(
                        Collection{} << MessageIds::message_response << received_string
                    );
                }
            );

            while(client->isConnected())
            {
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
        Remote::PingTime{4},
        Remote::PingTime{6} 
    };

    server.setIpVersion(nets::IPVersion::ipv4);
    server.setPort(60'000);

    server.startAccepting();

    while(true)
    {
    }

    return 0;
}