#include "../include/tcp_remote.hpp"

namespace nets
{
    TcpRemote::TcpRemote(boost::asio::io_context& io_context)
    :
        io_context{io_context},
        socket{io_context}
    {
    }

    const std::string& TcpRemote::getAddress() const 
    {
        return socket.remote_endpoint().address().to_string();
    }

    nets::Port TcpRemote::getPort() const 
    {
        return socket.remote_endpoint().port();
    }

    bool TcpRemote::connectionIsOpen()
    {
        return socket.is_open();
    }

    nets::TcpSocket& TcpRemote::getSocket()
    {
        return socket;
    }

    bool TcpRemote::operator==(const TcpRemote& remote)
    {
        return getAddress() == remote.getAddress() && getPort() == remote.getPort();
    }
}