#pragma once

#include "types.hpp"
#include "tcp_remote.hpp"

namespace nets
{
    template <typename MessageIdEnum, typename Remote = nets::TcpRemote<MessageIdEnum>>
    class TcpClient
    {
        public:
            using PingTime = Remote::PingTime;

            TcpClient(
                const std::string_view address,
                const std::string_view port,
                const PingTime         ping_timeout_period = PingTime{4},
                const PingTime         ping_delay          = PingTime{6}
            );

            ~TcpClient();

            void connect();
            bool disconnect();

            virtual void onConnection   (std::shared_ptr<Remote> server) = 0;
            virtual void onDisconnection(std::shared_ptr<Remote> server) = 0;
            virtual void process        (std::shared_ptr<Remote> server);

            std::string_view getServerAddress();
            nets::Port       getServerPort   ();

            bool isConnected();
        
        private:
            boost::asio::io_context io_context;

            const std::string address;
            const std::string port;

            std::shared_ptr<Remote> server;

            bool is_connected {false};

            std::atomic_bool active {true};
    };
}

// Implementation

namespace nets
{
    template <typename MessageIdEnum, typename Remote>
    TcpClient<MessageIdEnum, Remote>::TcpClient(
        const std::string_view  address,
        const std::string_view  port,
        const PingTime ping_timer,
        const PingTime ping_delay
    )
    :
        io_context{},

        address{address},
        port{port},

        server{std::make_shared<Remote>(io_context, ping_timer, ping_delay)}
    {
        std::thread {
            [this]
            {
                while(active.load())
                {
                    io_context.run();
                }
            }
        }.detach();        
    }

    template <typename MessageIdEnum, typename Remote>
    TcpClient<MessageIdEnum, Remote>::~TcpClient()
    {
        active = false;
    }

    template <typename MessageIdEnum, typename Remote>
    void TcpClient<MessageIdEnum, Remote>::connect()
    {
        if(!is_connected)
        {
            is_connected = true;

            boost::system::error_code error;

            boost::asio::ip::tcp::resolver resolver {io_context};

            boost::asio::connect(
                server->getSocket(),
                resolver.resolve(address, port),
                error
            );
            
            if(!error)
            {
                onConnection(server);
            }
        }
    }

    template <typename MessageIdEnum, typename Remote>
    bool TcpClient<MessageIdEnum, Remote>::disconnect()
    {
        if(is_connected)
        {
            boost::system::error_code error;

            server->getSocket().shutdown(TcpSocket::shutdown_both);
            server->getSocket().close(error);

            return !error;
        }
        else
        {
            return false;
        }
    }

    template <typename MessageIdEnum, typename Remote>
    void TcpClient<MessageIdEnum, Remote>::process(std::shared_ptr<Remote> server)
    {

    }
}