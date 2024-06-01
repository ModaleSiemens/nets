#pragma once

#include "types.hpp"

namespace nets
{
    class TcpRemote
    {
        public:
            //TcpRemote(nets::TcpSocket& socket);
            TcpRemote(boost::asio::io_context& io_context);

            std::string getAddress() const;
            nets::Port         getPort()    const;

            bool connectionIsOpen();

            nets::TcpSocket& getSocket();

            bool operator==(const TcpRemote& remote);

        private:
            boost::asio::io_context& io_context;
            nets::TcpSocket          socket;

            std::string address;
            nets::Port  port;
    };
}