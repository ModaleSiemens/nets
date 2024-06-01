#pragma once

#include <expected>
#include <chrono>
#include <atomic>

#include "types.hpp"
#include "../subprojects/collection/include/collection.hpp"

namespace nets
{
    class TcpRemote
    {
        public:
            using PingTime = std::chrono::duration<double, std::chrono::seconds>;

            //TcpRemote(nets::TcpSocket& socket);
            TcpRemote(boost::asio::io_context& io_context, const PingTime ping_timeout_period);

            void asyncSend(const mdsm::Collection& message);

            void onFailedSending(const mdsm::Collection& message) {};

            template <typename MessageIdEnum>
            void onReceiving(const MessageIdEnum message_id);

            void setPingingTimeoutPeriod(const PingTime period);

            void enablePinging();
            void disablePinging();

            virtual void onPingingTimeout() {};
            virtual void onSuccessfulPing() {};

            std::expected<PingTime, nets::PingError> ping(const PingTime period);

            std::string getAddress() const;
            nets::Port  getPort()    const;

            bool connectionIsOpen();

            nets::TcpSocket& getSocket();

            bool operator==(const TcpRemote& remote);

        private:
            boost::asio::io_context& io_context;
            nets::TcpSocket          socket;

            std::string address;
            nets::Port  port;

            std::atomic_bool pinging_enabled;
    };
}