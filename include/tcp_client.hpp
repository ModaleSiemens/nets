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
                const PingTime         ping_timeout_period       = PingTime{4},
                const PingTime         ping_delay                = PingTime{6}
            );

            virtual ~TcpClient();

            void connect();
            bool disconnect();

            virtual void onConnection(std::shared_ptr<Remote> server) = 0;

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

            boost::asio::executor_work_guard<decltype(io_context.get_executor())> io_context_work;
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

        server{
            std::make_shared<Remote>(
                io_context, ping_timer, ping_delay
            )
        },

        io_context_work{io_context.get_executor()}
    {
        std::thread {    
            [&, this]
            {
                io_context.run();
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
                server->start();
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

            server->stop();
            server->getSocket().shutdown(TcpSocket::shutdown_both);
            server->getSocket().close(error);

            return !error;
        }
        else
        {
            return false;
        }
    }
}