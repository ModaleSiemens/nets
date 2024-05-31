#include "../include/tcp_server.hpp"

#include "../include/tcp_remote.hpp"

namespace nets
{
    TcpServer::TcpServer(
        const nets::Port       port,
        const nets::IPVersion  ip_version
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
        }
    {
    }

    TcpServer::TcpServer(
        const nets::Port       port,
        const nets::IPVersion  ip_version,
        const std::string_view address
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
        }
    {
    }    

    bool TcpServer::startAccepting()
    {
        if(!is_accepting)
        {
            accept();

            return is_accepting = true;
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
            clients.push_back({io_context});

            acceptor.async_accept(
                clients.back().getSocket(),
                std::bind(
                    &TcpServer::handleAccepting,
                    this,
                    clients.back(),
                    std::placeholders::_1
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
            client.getSocket().close();
        }

        clients.clear();
    }

    TcpServer::~TcpServer()
    {
        closeAllConnections();
    }
}   