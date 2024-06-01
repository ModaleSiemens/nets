#include "../include/tcp_server.hpp"
#include "../include/tcp_remote.hpp"

#include <functional>

namespace nets
{
    TcpServer::TcpServer(
        const nets::Port       port,
        const nets::IPVersion  ip_version,
        const nets::TcpRemote::PingTime ping_timeout_time
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
        ping_timeout_time{ping_timeout_time}
    {
        std::thread {
            [this]
            {
                io_context.run();
            }
        }.detach();
    }

    TcpServer::TcpServer(
        const nets::Port       port,
        const nets::IPVersion  ip_version,
        const std::string_view address,
        const nets::TcpRemote::PingTime ping_timeout_time
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
        ping_timeout_time{ping_timeout_time}
    {
    }    

    bool TcpServer::startAccepting()
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

    bool TcpServer::stopAccepting()
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

    void TcpServer::accept()
    {
        if(is_accepting)
        {
            clients.push_back({io_context, ping_timeout_time});

            acceptor.async_accept(
                clients.back().getSocket(),
                std::bind(
                    &TcpServer::handleAccepting,
                    this,
                    std::ref(clients.back()),
                    boost::asio::placeholders::error
                )
            );
        }
    }

    void TcpServer::handleAccepting(
        TcpRemote& client,
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

    size_t TcpServer::getClientsCount()
    {
        return clients.size();
    }

    std::list<nets::TcpRemote>& TcpServer::getClients()
    {
        return clients;
    }

    const std::list<nets::TcpRemote>& TcpServer::getClients() const
    {
        return clients;
    }    

    bool TcpServer::closeConnection(nets::TcpRemote& client)
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

    void TcpServer::closeAllConnections()
    {
        for(auto& client : clients)
        {
            client.getSocket().shutdown(TcpSocket::shutdown_both);
            client.getSocket().close();
        }

        clients.clear();
    }

    TcpServer::~TcpServer()
    {
        stopAccepting();
        closeAllConnections();
    }
}   