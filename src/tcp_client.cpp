#include "../include/tcp_client.hpp"

namespace nets
{
    TcpClient::TcpClient(
        const std::string_view  address,
        const std::string_view  port
    )
    :
        io_context{},

        address{address},
        port{port},

        server{io_context}
    {
    }

    bool TcpClient::connect()
    {
        if(!is_connected)
        {
            is_connected = true;

            boost::asio::ip::tcp::resolver resolver {io_context};

            boost::asio::connect(
                server.getSocket(),
                resolver.resolve(address, port)
            );

            onConnection(server);

            return true;
        }
        else 
        {
            return false;
        }
    }

    bool TcpClient::disconnect()
    {
        if(is_connected)
        {
            boost::system::error_code error;

            server.getSocket().close(error);

            return !error;
        }
        else
        {
            return false;
        }
    }
}