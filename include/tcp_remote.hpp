#pragma once

#include "types.hpp"

namespace nets
{
    class TcpRemote
    {
        public:
            TcpRemote(nets::TcpSocket& socket);

            std::string getAddress() const;
            nets::Port  getPort()    const;

            nets::TcpSocket& getSocket();
    };
}