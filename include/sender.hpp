#pragma once

#include "types.hpp"

namespace nets
{
    class Sender
    {
        public:
            Sender(nets::Remote& remote);

            void sends(const mdsm::Collection message);

            template <typename T>
            T receive(const mdsm::Collection message);

            template <typename T>
            T receive();
    };
}