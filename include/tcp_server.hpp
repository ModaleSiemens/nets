#pragma once

#include "types.hpp"

#include <list>

namespace nets
{
    class TcpServer
    {
        public:
            TcpServer(
                const nets::Port       port,
                const nets::IPVersion  ip_version
            );

            // Bind acceptor to specific address
            TcpServer(
                const nets::Port       port,
                const nets::IPVersion  ip_version,
                const std::string_view address
            );            

            TcpServer(const TcpServer&) = delete;

            TcpServer& operator=(const TcpServer&) = delete;

            void start();
            void stop();

            virtual void onClientConnection   (nets::TcpRemote& client);
            virtual void onClientDisconnection(const nets::TcpRemote& client);

            void closeConnection(nets::TcpRemote& client);
            void closeAllConnections();

            size_t getClientsCount();

            std::vector<nets::TcpRemote>       getClients();
            const std::vector<nets::TcpRemote> getClients() const;

            ~TcpServer();
        
        private:
            boost::asio::io_context        io_context;
            boost::asio::ip::tcp::acceptor acceptor;

            std::list<TcpRemote> clients;
    };
}