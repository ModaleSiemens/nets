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
                const std::string_view address                   = "",
                const std::string_view port                      = "",
                const PingTime         ping_timeout_period       = PingTime{4},
                const PingTime         ping_delay                = PingTime{6}
            );

            virtual ~TcpClient();

            bool connect();
            void disconnect();

            virtual void onConnection(std::shared_ptr<Remote> server) = 0;

            void setServerAddress(const std::string_view address);
            void setServerPort   (const std::string_view port);

            std::string_view getServerAddress();
            std::string_view getServerPort   ();
 
        private:
            boost::asio::io_context client_io_context;

            std::string address;
            std::string port;            

        public:
            std::shared_ptr<Remote> server;

        private:
            std::atomic_bool active {true};

            boost::asio::executor_work_guard<decltype(client_io_context.get_executor())> client_io_context_work;
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
        client_io_context{},

        address{address},
        port{port},

        server{
            std::make_shared<Remote>(
                client_io_context, ping_timer, ping_delay
            )
        },

        client_io_context_work{client_io_context.get_executor()}
    {
        std::thread {    
            [&, this]
            {
                client_io_context.run();
            }
        }.detach();
    }

    template <typename MessageIdEnum, typename Remote>
    TcpClient<MessageIdEnum, Remote>::~TcpClient()
    {
        active = false;
    }

    template <typename MessageIdEnum, typename Remote>
    bool TcpClient<MessageIdEnum, Remote>::connect()
    {
        boost::system::error_code error;

        boost::asio::ip::tcp::resolver resolver {client_io_context};

        boost::asio::connect(
            server->getSocket(),
            resolver.resolve(address, port),
            error
        );
        
        if(!error)
        {
            server->start();

            std::thread {
                &TcpClient::onConnection, this, server
            }.detach();

            return true;   
        }
        else
        {
            return false;
        }
    }

    template <typename MessageIdEnum, typename Remote>
    void TcpClient<MessageIdEnum, Remote>::disconnect()
    {
        boost::system::error_code error;

        server->stop();
        server->getSocket().shutdown(TcpSocket::shutdown_both);
        server->getSocket().close(error);
    }

    template <typename MessageIdEnum, typename Remote>
    void TcpClient<MessageIdEnum, Remote>::setServerAddress(const std::string_view t_address)
    {
        address = t_address;
    }
    
    template <typename MessageIdEnum, typename Remote>
    void TcpClient<MessageIdEnum, Remote>::setServerPort(const std::string_view t_port)
    {
        port = t_port;
    }

    template <typename MessageIdEnum, typename Remote>
    std::string_view TcpClient<MessageIdEnum, Remote>::getServerAddress()
    {
        return address;
    }

    template <typename MessageIdEnum, typename Remote>
    std::string_view TcpClient<MessageIdEnum, Remote>::getServerPort()
    {
        return port;
    }    
}
