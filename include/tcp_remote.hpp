#pragma once

#include <expected>
#include <chrono>
#include <atomic>
#include <functional>
#include <unordered_map>

#include "types.hpp"
#include "../subprojects/collection/include/collection.hpp"

namespace nets
{
    template <typename MessageIdEnum>
    class TcpRemote
    {
        public:
            using PingTime = std::chrono::duration<double, std::chrono::seconds>;
            using MessageReceivedCallback = std::function<void(const mdsm::Collection& collection, TcpRemote& remote)>;

            //TcpRemote(nets::TcpSocket& socket);
            TcpRemote(boost::asio::io_context& io_context, const PingTime ping_timeout_period);

            void asyncSend(const mdsm::Collection& message);

            void onFailedSending(const mdsm::Collection& message) {};

            void setOnReceiving(const MessageIdEnum message_id, const MessageReceivedCallback& callback);

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

            PingTime ping_timeout_period;

            std::unordered_map<MessageIdEnum, MessageReceivedCallback> message_callbacks;
    };
}

// Implementation

namespace nets
{
    template <typename MessageIdEnum>
    TcpRemote<MessageIdEnum>::TcpRemote(boost::asio::io_context& io_context, const PingTime ping_timeout_period)
    :
        io_context{io_context},
        socket{io_context},
        ping_timeout_period{ping_timeout_period}
    {
    }
    
    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::setPingingTimeoutPeriod(const PingTime t_ping_timeout_period)
    {
        ping_timeout_period = t_ping_timeout_period;
    }

    template <typename MessageIdEnum>
    std::string TcpRemote<MessageIdEnum>::getAddress() const 
    {
        return socket.remote_endpoint().address().to_string();
    }

    template <typename MessageIdEnum>
    nets::Port TcpRemote<MessageIdEnum>::getPort() const 
    {
        return socket.remote_endpoint().port();
    }

    template <typename MessageIdEnum>
    bool TcpRemote<MessageIdEnum>::connectionIsOpen()
    {
        std::string empty_string {""};

        boost::system::error_code error;

        boost::asio::read(socket, boost::asio::buffer(empty_string), error);

        return !error;
    }

    template <typename MessageIdEnum>
    nets::TcpSocket& TcpRemote<MessageIdEnum>::getSocket()
    {
        return socket;
    }

    template <typename MessageIdEnum>
    bool TcpRemote<MessageIdEnum>::operator==(const TcpRemote& remote)
    {
        return getAddress() == remote.getAddress() && getPort() == remote.getPort();
    }
}