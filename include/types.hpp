#pragma once

#include <boost/asio.hpp>

namespace nets
{  
    enum class Protocol 
    {
        TCP, UDP
    };

    using Port = boost::asio::ip::port_type;

    template <Protocol protocol>
    class Server;
    class Client;
    
    class Remote;
}
