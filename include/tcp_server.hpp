#pragma once

#include "types.hpp"
#include "../include/tcp_remote.hpp"

#include <functional>
#include <list>

namespace nets
{
    template <typename MessageIdEnum, typename Remote = nets::TcpRemote<MessageIdEnum>>
    class TcpServer
    {
        public:
            using PingTime = Remote::PingTime;

            TcpServer(
                const nets::Port                port,
                const nets::IPVersion           ip_version,
                const PingTime ping_timeout_period = PingTime{2},
                const PingTime ping_delay          = PingTime{4},
                const bool     enable_pinging            = true,
                const bool     enable_being_pinged       = true,
                const bool     enable_receiving_messages = true                
            );

            // Bind acceptor to specific address
            TcpServer(
                const nets::Port                port,
                const nets::IPVersion           ip_version,
                const std::string_view          address,
                const PingTime ping_timeout_period = PingTime{2},
                const PingTime ping_delay          = PingTime{4},
                const bool     enable_pinging            = true,
                const bool     enable_being_pinged       = true,
                const bool     enable_receiving_messages = true                 
            );            

            TcpServer(const TcpServer&) = delete;

            TcpServer& operator=(const TcpServer&) = delete;

            bool startAccepting();
            bool stopAccepting ();

            virtual void onClientConnection(std::shared_ptr<Remote> client) = 0;
            
            // Client connected when server wasn't accepting requests
            virtual void onForbiddenClientConnection(std::shared_ptr<Remote> client) = 0; 

            bool closeConnection(std::shared_ptr<Remote> client);
            void closeAllConnections();

            size_t getClientsCount();

            std::list<std::shared_ptr<Remote>>&       getClients();
            const std::list<std::shared_ptr<Remote>>& getClients() const;

            ~TcpServer();
        
        private:
            boost::asio::io_context        io_context;
            boost::asio::ip::tcp::acceptor acceptor;

            std::vector<std::shared_ptr<Remote>> clients;
            
            bool is_accepting {false};

            PingTime ping_timeout_time;
            PingTime ping_delay;

            bool enable_pinging;
            bool enable_being_pinged;
            bool enable_receiving_messages;

            void accept();

            void handleAccepting(
                std::shared_ptr<Remote>   client,
                boost::system::error_code error,
                nets::TcpSocket           socket
            );

            std::atomic_bool active {true};

            boost::asio::executor_work_guard<decltype(io_context.get_executor())> io_context_work;
    };
}

// Implementation

namespace nets
{
    template <typename MessageIdEnum, typename Remote>
    TcpServer<MessageIdEnum, Remote>::TcpServer(
        const nets::Port       port,
        const nets::IPVersion  ip_version,
        const PingTime ping_timeout_time,
        const PingTime ping_delay,
        const bool enable_pinging,
        const bool enable_being_pinged,
        const bool enable_receiving_messages           
    )
    :
        io_context{},
        acceptor {
            io_context,
            boost::asio::ip::tcp::endpoint
            {
                ip_version == IPVersion::ipv4 ? 
                boost::asio::ip::tcp::v4() : boost::asio::ip::tcp::v6(),
                port
            }
        },
        ping_timeout_time{ping_timeout_time},
        ping_delay{ping_delay},
        enable_pinging{enable_pinging},
        enable_being_pinged{enable_being_pinged},
        enable_receiving_messages{enable_receiving_messages},
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
    TcpServer<MessageIdEnum, Remote>::TcpServer(
        const nets::Port       port,
        const nets::IPVersion  ip_version,
        const std::string_view address,
        const PingTime ping_timeout_time,
        const PingTime ping_delay,
        const bool enable_pinging,
        const bool enable_being_pinged,
        const bool enable_receiving_messages           
    )
    :
        io_context{},
        acceptor {
            io_context,
            boost::asio::ip::tcp::endpoint
            {
                boost::asio::ip::make_address(address),
                port
            }
        },
        ping_timeout_time{ping_timeout_time},
        ping_delay{ping_delay},
        enable_pinging{enable_pinging},
        enable_being_pinged{enable_being_pinged},
        enable_receiving_messages{enable_receiving_messages}        
    {
    }    

    template <typename MessageIdEnum, typename Remote>
    bool TcpServer<MessageIdEnum, Remote>::startAccepting()
    {
        if(!is_accepting)
        {
            is_accepting = true;

            accept();

            return true;
        }
        else 
        {
            return false;
        }
    }

    template <typename MessageIdEnum, typename Remote>
    bool TcpServer<MessageIdEnum, Remote>::stopAccepting()
    {
        if(is_accepting)
        {
            boost::system::error_code error;

            if(acceptor.is_open())
            {
                acceptor.close(error);
            }

            return !(is_accepting = false) && !error;
        }
        else 
        {
            return false;
        }
    }    

    template <typename MessageIdEnum, typename Remote>
    void TcpServer<MessageIdEnum, Remote>::accept()
    {
        if(is_accepting)
        {
            std::println("DEBUG: Accepting");

            clients.emplace_back(
                std::make_shared<Remote>(
                    io_context, ping_timeout_time, ping_delay,
                    enable_pinging, enable_being_pinged, enable_receiving_messages
                )
            ); 

            acceptor.async_accept(
                boost::asio::make_strand(acceptor.get_executor()),
                std::bind(
                    &TcpServer<MessageIdEnum, Remote>::handleAccepting,
                    this,
                    clients.back(),
                    std::placeholders::_1,
                    std::placeholders::_2
                )
            );
        }
    }

    template <typename MessageIdEnum, typename Remote>
    void TcpServer<MessageIdEnum, Remote>::handleAccepting(
        std::shared_ptr<Remote> client,
        boost::system::error_code error,
        nets::TcpSocket socket
    )
    {
        if(!error)
        {
            client->getSocket() = std::move(socket);

            if(is_accepting)
            {
                std::println("DEBUG: Accepted connection");
                accept();

                client->start();
                onClientConnection(client);
            }
            else
            {
                onForbiddenClientConnection(client);
            }
        }
        else
        {
            // Error occourred
        }
    }

    template <typename MessageIdEnum, typename Remote>
    size_t TcpServer<MessageIdEnum, Remote>::getClientsCount()
    {
        return clients.size();
    }

    template <typename MessageIdEnum, typename Remote>
    std::list<std::shared_ptr<Remote>>& TcpServer<MessageIdEnum, Remote>::getClients()
    {
        return clients;
    }

    template <typename MessageIdEnum, typename Remote>
    const std::list<std::shared_ptr<Remote>>& TcpServer<MessageIdEnum, Remote>::getClients() const
    {
        return clients;
    }    

    template <typename MessageIdEnum, typename Remote>
    bool TcpServer<MessageIdEnum, Remote>::closeConnection(std::shared_ptr<Remote> client)
    {
        const auto client_iter {
            std::find(clients.begin(), clients.end(), client)
        };

        if(client_iter != clients.end())
        {
            boost::system::error_code error;

            (*client_iter)->getSocket().shutdown(TcpSocket::shutdown_both);
            (*client_iter)->getSocket().close(error);

            clients.erase(client_iter);

            std::println("DEBUG: Closing connection");

            return !error;
        } 
        else
        {
            return false;
        }
    }

    template <typename MessageIdEnum, typename Remote>
    void TcpServer<MessageIdEnum, Remote>::closeAllConnections()
    {
        for(auto& client : clients)
        {
            client->getSocket().shutdown(TcpSocket::shutdown_both);
            client->getSocket().close();
        }

        clients.clear();
    }

    template <typename MessageIdEnum, typename Remote>
    TcpServer<MessageIdEnum, Remote>::~TcpServer()
    {
        stopAccepting();
        closeAllConnections();

        active = false;
    }
}   