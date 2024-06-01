#pragma once

#include "types.hpp"
#include "tcp_remote.hpp"

namespace nets
{
    template <typename MessageIdEnum>
    class TcpClient
    {
        public:
            using PingTime = TcpRemote<MessageIdEnum>::PingTime;

            TcpClient(
                const std::string_view          address,
                const std::string_view          port,
                const PingTime ping_timeout_period = PingTime{2},
                const PingTime ping_delay          = PingTime{4}
            );

            bool connect();
            bool disconnect();

            virtual void onConnection(nets::TcpRemote<MessageIdEnum>& server) = 0;
            virtual void onDisconnection(nets::TcpRemote<MessageIdEnum>& server) = 0;
            virtual void process(nets::TcpRemote<MessageIdEnum>& server);

            std::string_view getServerAddress();
            nets::Port       getServerPort   ();

            bool isConnected();
        
        private:
            boost::asio::io_context io_context;

            const std::string address;
            const std::string port;

            nets::TcpRemote<MessageIdEnum> server;

            bool is_connected {false};
    };
}

// Implementation

namespace nets
{
    template <typename MessageIdEnum>
    TcpClient<MessageIdEnum>::TcpClient(
        const std::string_view  address,
        const std::string_view  port,
        const PingTime ping_timer,
        const PingTime ping_delay
    )
    :
        io_context{},

        address{address},
        port{port},

        server{io_context, ping_timer, ping_delay}
    {
    }

    template <typename MessageIdEnum>
    bool TcpClient<MessageIdEnum>::connect()
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

    template <typename MessageIdEnum>
    bool TcpClient<MessageIdEnum>::disconnect()
    {
        if(is_connected)
        {
            boost::system::error_code error;

            server.getSocket().shutdown(TcpSocket::shutdown_both);
            server.getSocket().close(error);

            return !error;
        }
        else
        {
            return false;
        }
    }

    template <typename MessageIdEnum>
    void TcpClient<MessageIdEnum>::process(nets::TcpRemote<MessageIdEnum>& server)
    {

    }
}