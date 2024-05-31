#pragma once

#include <boost/asio.hpp>

namespace nets
{
    using Port = boost::asio::ip::port_type;

    using TcpSocket = boost::asio::ip::tcp::socket;
    using UdpSocket = boost::asio::ip::udp::socket;

    enum class AddressKind
    {
        dns, raw
    };

    enum class IPVersion
    {
        ipv4, ipv6
    };

    class Server;
    class Client;

    class TcpRemote;
    class UdpRemote;

    class TcpSender;
}
