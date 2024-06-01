#pragma once

#include "types.hpp"
#include "tcp_remote.hpp"

namespace nets
{
    class TcpClient
    {
        public:
            TcpClient(
                const std::string_view  address,
                const std::string_view  port
            );

            bool connect();
            bool disconnect();

            virtual void onConnection(nets::TcpRemote& server) = 0;

            std::string_view getServerAddress();
            nets::Port       getServerPort   ();

            bool isConnected();
        
        private:
            boost::asio::io_context io_context;

            const std::string address;
            const std::string port;

            nets::TcpRemote server;

            bool is_connected {false};
    };
}