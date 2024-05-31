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

            bool startAccepting();
            bool stopAccepting ();

            virtual void onClientConnection         (nets::TcpRemote& client);
            virtual void onForbiddenClientConnection(nets::TcpRemote& client);
            virtual void onClientDisconnection(const nets::TcpRemote& client);

            void closeConnection(nets::TcpRemote& client);
            void closeAllConnections();

            size_t getClientsCount();

            std::list<nets::TcpRemote>&       getClients();
            const std::list<nets::TcpRemote>& getClients() const;

            ~TcpServer();
        
        private:
            boost::asio::io_context        io_context;
            boost::asio::ip::tcp::acceptor acceptor;

            std::list<TcpRemote> clients;
            
            bool is_accepting {false};

            void accept();

            void handleAccepting(
                TcpRemote& client,
                const boost::system::error_code& error
            );
    };
}