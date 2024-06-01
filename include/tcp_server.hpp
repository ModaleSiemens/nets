#pragma once

#include "types.hpp"

#include <list>

namespace nets
{
    class TcpServer
    {
        public:
            TcpServer(
                const nets::Port                port,
                const nets::IPVersion           ip_version,
                const nets::TcpRemote::PingTime ping_timeout_time = nets::TcpRemote::PingTime{3}
            );

            // Bind acceptor to specific address
            TcpServer(
                const nets::Port                port,
                const nets::IPVersion           ip_version,
                const std::string_view          address,
                const nets::TcpRemote::PingTime ping_timeout_time = nets::TcpRemote::PingTime{3}

            );            

            TcpServer(const TcpServer&) = delete;

            TcpServer& operator=(const TcpServer&) = delete;

            bool startAccepting();
            bool stopAccepting ();

            virtual void onClientConnection(nets::TcpRemote& client) = 0;
            
            // Client connected when server wasn't accepting requests
            virtual void onForbiddenClientConnection(nets::TcpRemote& client) = 0; 

            bool closeConnection(nets::TcpRemote& client);
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

            nets::TcpRemote::PingTime ping_timeout_time;

            void accept();

            void handleAccepting(
                TcpRemote& client,
                const boost::system::error_code& error
            );
    };
}