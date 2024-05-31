#include "../include/tcp_server.hpp"

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

    void TcpServer::start()
    {
        clients.push_back({io_context});
    }
}   