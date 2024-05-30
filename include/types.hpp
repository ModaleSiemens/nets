#pragma once

#include <boost/asio.hpp>

namespace nets
{
    using Port = boost::asio::ip::port_type;

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

    class Remote;

    class Sender;
}
