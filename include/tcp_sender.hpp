#pragma once

#include "types.hpp"

#include "../subprojects/collection/include/collection.hpp"

namespace nets
{
    class TcpSender
    {
        public:
            TcpSender(nets::TcpRemote& remote);

            void sends(const mdsm::Collection message);

            template <typename T>
            T receive(const mdsm::Collection message);

            template <typename T>
            T receive();
    };
}