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
                const std::string_view address = ""
            );

            TcpServer(const TcpServer&) = delete;

            TcpServer& operator=(const TcpServer&) = delete;

            void start();
            void stop();

            virtual void onClientConnection   (nets::Remote& client);
            virtual void onClientDisconnection(const nets::Remote& client);

            void closeConnection(nets::Remote& client);
            void closeAllConnections();

            size_t getClientsCount();

            std::vector<nets::Remote>       getClients();
            const std::vector<nets::Remote> getClients() const;

            ~TcpServer();
        
        private:
            // TODO 
    };
}