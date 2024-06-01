#include "../include/tcp_client.hpp"

namespace nets
{
    TcpClient::TcpClient(
        const std::string_view  address,
        const std::string_view  port,
        const nets::TcpRemote::PingTime ping_timer
    )
    :
        io_context{},

        address{address},
        port{port},

        server{io_context, ping_timer}
    {
    }

    bool TcpClient::connect()
    {
        if(!is_connected)
        {
            is_connected = true;

            boost::asio::ip::tcp::resolver resolver {io_context};

            boost::system::error_code error;

            boost::asio::connect(
                server.getSocket(),
                resolver.resolve(address, port),
                error
            );
            
            if(!error)
            {
                onConnection(server);

                return true;
            }
            else
            {
                return false;
            }

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

    void TcpClient::process(nets::TcpRemote& server)
    {

    }
}