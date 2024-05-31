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
            nets::Port  getPort()    const;

            bool connectionIsOpen();

            nets::TcpSocket& getSocket();

        private:
            nets::TcpSocket socket;
    };
}