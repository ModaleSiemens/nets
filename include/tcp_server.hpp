#pragma once

#include "types.hpp"

#include <vector>

namespace nets
{
    class TcpServer
    {
        public:
            TcpServer(
                const nets::Port       port,
                const nets::IPVersion  ip_version,
                const size_t           queue_size,
                const std::string_view address = ""
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
            // TODO 
    };
}