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
                const PingTime ping_timeout_period = PingTime{2},
                const PingTime ping_delay          = PingTime{4}           
            );          

            TcpServer(const TcpServer&) = delete;

            TcpServer& operator=(const TcpServer&) = delete;

            bool startAccepting();
            bool stopAccepting ();

            void setAddress  (const std::string_view address);
            void setIpVersion(const nets::IPVersion  ip_version);
            void setPort     (const nets::Port       port);

            std::string_view getAddress();
            nets::IPVersion  getIpVersion();
            nets::Port       getPort();

            virtual void onClientConnection(std::shared_ptr<Remote> client) = 0;
            
            // Client connected when server wasn't accepting requests
            virtual void onForbiddenClientConnection(std::shared_ptr<Remote> client) = 0; 

            bool closeConnection(std::shared_ptr<Remote> client);
            void closeAllConnections();

            size_t getClientsCount();

            std::vector<std::shared_ptr<Remote>>&       getClients();
            const std::vector<std::shared_ptr<Remote>>& getClients() const;

            virtual ~TcpServer();
        
        private:
            boost::asio::io_context        server_io_context;
            std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor;

            std::string address;
            IPVersion   ip_version;
            nets::Port  port;

            std::vector<std::shared_ptr<Remote>> clients;
            
            bool is_accepting {false};

            PingTime ping_timeout_time;
            PingTime ping_delay;

            void accept();

            void handleAccepting(
                std::shared_ptr<Remote>   client,
                boost::system::error_code error,
                nets::TcpSocket           socket
            );

            std::atomic_bool active {true};

            boost::asio::executor_work_guard<decltype(server_io_context.get_executor())> server_io_context_work;
    };
}

// Implementation

namespace nets
{
    template <typename MessageIdEnum, typename Remote>
    TcpServer<MessageIdEnum, Remote>::TcpServer(
        const PingTime ping_timeout_time,
        const PingTime ping_delay
    )
    :
        server_io_context{},
        ping_timeout_time{ping_timeout_time},
        ping_delay{ping_delay},
        server_io_context_work{server_io_context.get_executor()}
    {
        std::thread {    
            [&, this]
            {
                server_io_context.run();
            }
        }.detach();
    }  

    template <typename MessageIdEnum, typename Remote>
    bool TcpServer<MessageIdEnum, Remote>::startAccepting()
    {
        if(!is_accepting)
        {
            is_accepting = true;

            if(address == "")
            {
                acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(
                    server_io_context,
                    boost::asio::ip::tcp::endpoint{
                        ip_version == IPVersion::ipv4 ? boost::asio::ip::tcp::v4() : boost::asio::ip::tcp::v6(),
                        port
                    }
                );
            }
            else 
            {
                acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(
                    server_io_context,
                    boost::asio::ip::tcp::endpoint{
                        boost::asio::ip::make_address(address),
                        port
                    }
                );
            }

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

            if(acceptor->is_open())
            {
                acceptor->close(error);
            }

            return !(is_accepting = false) && !error;
        }
        else 
        {
            return false;
        }
    }

    template <typename MessageIdEnum, typename Remote>
    void TcpServer<MessageIdEnum, Remote>::setAddress(const std::string_view t_address)
    {
        address = t_address;
    }

    template <typename MessageIdEnum, typename Remote>
    void TcpServer<MessageIdEnum, Remote>::setIpVersion(const nets::IPVersion t_ip_version)
    {
        ip_version = t_ip_version;
    }

    template <typename MessageIdEnum, typename Remote>
    void TcpServer<MessageIdEnum, Remote>::setPort(const nets::Port t_port)
    {
        port = t_port;
    }

    template <typename MessageIdEnum, typename Remote>
    std::string_view TcpServer<MessageIdEnum, Remote>::getAddress()
    {
        return address;
    }

    template <typename MessageIdEnum, typename Remote>
    nets::IPVersion TcpServer<MessageIdEnum, Remote>::getIpVersion()
    {
        return ip_version;
    }

    template <typename MessageIdEnum, typename Remote>
    nets::Port TcpServer<MessageIdEnum, Remote>::getPort()
    {
        return port;
    }

    template <typename MessageIdEnum, typename Remote>
    void TcpServer<MessageIdEnum, Remote>::accept()
    {
        if(is_accepting)
        {
            //std::println("DEBUG: Accepting");

            clients.push_back(
                std::make_shared<Remote>(
                    server_io_context, ping_timeout_time, ping_delay
                )
            ); 

            acceptor->async_accept(
                boost::asio::make_strand(acceptor->get_executor()),
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
                //std::println("DEBUG: Accepted connection");
                accept();

                client->start();          
                
                std::thread {
                    &TcpServer::onClientConnection, this, client
                }.detach();
            }
            else
            {
                std::thread {
                    &TcpServer::onForbiddenClientConnection, this, client
                }.detach();
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
    std::vector<std::shared_ptr<Remote>>& TcpServer<MessageIdEnum, Remote>::getClients()
    {
        return clients;
    }

    template <typename MessageIdEnum, typename Remote>
    const std::vector<std::shared_ptr<Remote>>& TcpServer<MessageIdEnum, Remote>::getClients() const
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

            //std::println("DEBUG: Closing connection");

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