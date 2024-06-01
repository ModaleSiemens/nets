#pragma once

#include <boost/asio.hpp>

namespace nets
{
    using Port = boost::asio::ip::port_type;

    using TcpSocket = boost::asio::ip::tcp::socket;

    enum class IPVersion
    {
        ipv4, ipv6
    };

    enum class PingError
    {
        espired, failed_to_send
    };

    template <typename MessageIdEnum>
    class TcpServer;

    template <typename MessageIdEnum>
    class TcpClient;

    template <typename MessageIdEnum>
    class TcpRemote;
}
