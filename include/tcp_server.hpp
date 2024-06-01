#pragma once

#include "types.hpp"
#include "../include/tcp_remote.hpp"

#include <functional>
#include <list>

namespace nets
{
    template <typename MessageIdEnum>
    class TcpServer
    {
        public:
            TcpServer(
                const nets::Port                port,
                const nets::IPVersion           ip_version,
                const nets::TcpRemote<MessageIdEnum>::PingTime ping_timeout_period = TcpRemote<MessageIdEnum>::PingTime{2},
                const nets::TcpRemote<MessageIdEnum>::PingTime ping_delay          = TcpRemote<MessageIdEnum>::PingTime{4}
            );

            // Bind acceptor to specific address
            TcpServer(
                const nets::Port                port,
                const nets::IPVersion           ip_version,
                const std::string_view          address,
                const nets::TcpRemote<MessageIdEnum>::PingTime ping_timeout_period = TcpRemote<MessageIdEnum>::PingTime{2},
                const nets::TcpRemote<MessageIdEnum>::PingTime ping_delay          = TcpRemote<MessageIdEnum>::PingTime{4}
            );            

            TcpServer(const TcpServer&) = delete;

            TcpServer& operator=(const TcpServer&) = delete;

            bool startAccepting();
            bool stopAccepting ();

            virtual void onClientConnection(nets::TcpRemote<MessageIdEnum>& client) = 0;
            
            // Client connected when server wasn't accepting requests
            virtual void onForbiddenClientConnection(nets::TcpRemote<MessageIdEnum>& client) = 0; 

            bool closeConnection(nets::TcpRemote<MessageIdEnum>& client);
            void closeAllConnections();

            size_t getClientsCount();

            std::list<nets::TcpRemote<MessageIdEnum>>&       getClients();
            const std::list<nets::TcpRemote<MessageIdEnum>>& getClients() const;

            ~TcpServer();
        
        private:
            boost::asio::io_context        io_context;
            boost::asio::ip::tcp::acceptor acceptor;

            std::list<TcpRemote<MessageIdEnum>> clients;
            
            bool is_accepting {false};

            nets::TcpRemote<MessageIdEnum>::PingTime ping_timeout_time;
            nets::TcpRemote<MessageIdEnum>::PingTime ping_delay;

            void accept();

            void handleAccepting(
                TcpRemote<MessageIdEnum>& client,
                const boost::system::error_code& error
            );
    };
}

// Implementation

namespace nets
{
    template <typename MessageIdEnum>
    TcpServer<MessageIdEnum>::TcpServer(
        const nets::Port       port,
        const nets::IPVersion  ip_version,
        const nets::TcpRemote<MessageIdEnum>::PingTime ping_timeout_time,
        const nets::TcpRemote<MessageIdEnum>::PingTime ping_delay
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
        ping_delay{ping_delay}
    {
        std::thread {
            [this]
            {
                io_context.run();
            }
        }.detach();
    }

    template <typename MessageIdEnum>
    TcpServer<MessageIdEnum>::TcpServer(
        const nets::Port       port,
        const nets::IPVersion  ip_version,
        const std::string_view address,
        const nets::TcpRemote<MessageIdEnum>::PingTime ping_timeout_time,
        const nets::TcpRemote<MessageIdEnum>::PingTime ping_delay
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
        ping_delay{ping_delay}
    {
    }    

    template <typename MessageIdEnum>
    bool TcpServer<MessageIdEnum>::startAccepting()
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

    template <typename MessageIdEnum>
    bool TcpServer<MessageIdEnum>::stopAccepting()
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

    template <typename MessageIdEnum>
    void TcpServer<MessageIdEnum>::accept()
    {
        if(is_accepting)
        {
            clients.push_back({io_context, ping_timeout_time});

            acceptor.async_accept(
                clients.back().getSocket(),
                std::bind(
                    &TcpServer<MessageIdEnum>::handleAccepting,
                    this,
                    std::ref(clients.back()),
                    boost::asio::placeholders::error
                )
            );
        }
    }

    template <typename MessageIdEnum>
    void TcpServer<MessageIdEnum>::handleAccepting(
        TcpRemote<MessageIdEnum>& client,
        const boost::system::error_code& error
    )
    {
        if(!error)
        {
            if(is_accepting)
            {
                onClientConnection(client);
            }
            else
            {
                onForbiddenClientConnection(client);
            }
        }

        accept();
    }

    template <typename MessageIdEnum>
    size_t TcpServer<MessageIdEnum>::getClientsCount()
    {
        return clients.size();
    }

    template <typename MessageIdEnum>
    std::list<nets::TcpRemote<MessageIdEnum>>& TcpServer<MessageIdEnum>::getClients()
    {
        return clients;
    }

    template <typename MessageIdEnum>
    const std::list<nets::TcpRemote<MessageIdEnum>>& TcpServer<MessageIdEnum>::getClients() const
    {
        return clients;
    }    

    template <typename MessageIdEnum>
    bool TcpServer<MessageIdEnum>::closeConnection(nets::TcpRemote<MessageIdEnum>& client)
    {
        const auto client_iter {
            std::find(clients.begin(), clients.end(), client)
        };

        if(client_iter != clients.end())
        {
            boost::system::error_code error;

            client_iter->getSocket().shutdown(TcpSocket::shutdown_both);
            client_iter->getSocket().close(error);

            clients.erase(client_iter);

            return !error;
        } 
        else
        {
            return false;
        }
    }

    template <typename MessageIdEnum>
    void TcpServer<MessageIdEnum>::closeAllConnections()
    {
        for(auto& client : clients)
        {
            client.getSocket().shutdown(TcpSocket::shutdown_both);
            client.getSocket().close();
        }

        clients.clear();
    }

    template <typename MessageIdEnum>
    TcpServer<MessageIdEnum>::~TcpServer()
    {
        stopAccepting();
        closeAllConnections();
    }
}   