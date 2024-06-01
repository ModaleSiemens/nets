#pragma once

#include "types.hpp"
#include "tcp_remote.hpp"

namespace nets
{
    template <template MessageIdEnum>
    class TcpClient
    {
        public:
            TcpClient(
                const std::string_view          address,
                const std::string_view          port,
                const nets::TcpRemote::PingTime ping_timer = TcpRemote::PingTime{3}
            );

            bool connect();
            bool disconnect();

            virtual void onConnection(nets::TcpRemote& server) = 0;
            virtual void onDisconnection(nets::TcpRemote& server) = 0;
            virtual void process(nets::TcpRemote& server);

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

// Implementation

namespace nets
{
    template <template MessageIdEnum>
    TcpClient::TcpClient(
        const std::string_view  address,
        const std::string_view  port,
        const nets::TcpRemote::PingTime ping_timer
    )
    :
        io_context{},

        address{address},
        port{port},

        server{io_context, ping_timer}
    {
    }

    template <template MessageIdEnum>
    bool TcpClient::connect()
    {
        if(!is_connected)
        {
            is_connected = true;

            boost::asio::ip::tcp::resolver resolver {io_context};

            boost::system::error_code error;

            boost::asio::connect(
                server.getSocket(),
                resolver.resolve(address, port),
                error
            );
            
            if(!error)
            {
                onConnection(server);

                return true;
            }
            else
            {
                return false;
            }

            return true;
        }
        else 
        {
            return false;
        }
    }

    template <template MessageIdEnum>
    bool TcpClient::disconnect()
    {
        if(is_connected)
        {
            boost::system::error_code error;

            server.getSocket().close(error);

            return !error;
        }
        else
        {
            return false;
        }
    }

    template <template MessageIdEnum>
    void TcpClient::process(nets::TcpRemote& server)
    {

    }
}