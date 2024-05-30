#pragma once

#include "types.hpp"

#include <string_view>

namespace nets
{
    template <>
    class Server<Protocol::TCP>
    {
        public:
            Server(const Port port, const std::string_view address = "");
            
            virtual ~Server();

            void start();
            void stop ();

            virtual void onClientConnection   (      Remote& client);
            virtual void onClientDisconnection(const Remote& client);

            void disconnectAll();

            void disconnect(Remote& client);

            std::vector<Remote>&       getClients();
            const std::vector<Remote>& getClients() const;

        private:
            boost::asio::ip::tcp::endpoint endpoint;

            std::vector<Remote> clients;
    };  

    Server<Protocol::TCP>::Server(const Port port, const std::string_view address)
    :   
        // If address is empty, create an endpoint from all local addresses
        endpoint{
            address == "" ? boost::asio::ip::address_v6::any() : boost::asio::ip::make_address(address),
            port
        }
    {
    }
}