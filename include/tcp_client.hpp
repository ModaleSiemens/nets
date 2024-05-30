#pragma once

#include "types.hpp"

namespace nets
{
    class TcpClient
    {
        public:
            TcpClient(
                const std::string_view  address,
                const nets::Port        port,
                const nets::AddressKind address_kind
            );

            void connect   ();
            void disconnect();

            virtual void onConnection   (nets::Remote& server);
            virtual void onDisconnection(const nets::Remote& server);

            std::string_view getServerAddress();
            nets::Port       getServerPort   ();

    };
}