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

            void startListeningForIncomingMessages();
            void stopListeningForIncomingMessages();

            void startPinging();
            void stopPinging();

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

            std::atomic_bool pinging_enabled    {false};
            std::atomic_bool listening_enabled  {true};

            PingTime ping_timeout_period;

            std::unordered_map<MessageIdEnum, MessageReceivedCallback> message_callbacks;

            std::vector<std::bytes> read_message_size (sizeof(mdsm::Collection::getSize()));
            mdsm::Collection        read_message_data;
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

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::asyncSend(const mdsm::Collection& message)
    {
        const auto message_size {message.getSize()};

        std::vector<std::byte> message_with_header (sizeof(message_size) + message_size);

        // Transpose data to specific endianness
        const auto prepared_message_size {message.prepareDataForInserting(message_size)};

        std::memcpy(message_with_header.data(), prepared_message_size.data(), sizeof(message_size));
        std::memcpy(
            message_with_header.data() + sizeof(message_size),
            message.getData(),
            message_size
        );

        boost::asio::async_write(
            socket,
            asio::buffer(message_with_header.data(), message_with_header.size()),
            [&, this](const boost::system::error_code& error, const std::size_t bytes_count)
            {
                if(error)
                {
                    onFailedSending(message);
                }
            }
        );
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::startListeningForIncomingMessages()
    {
        boost::asio::async_read(
            socket,
            boost::asio::buffer(&read_message_size, sizeof(read_message_size)),
            [&, this](const boost::system::error_code error, const std::size_t bytes_count)
            {
                if(listening_enabled.load())
                {
                    const auto message_size {
                        mdsm::Collection::prepareDataForExtracting<decltype(mdsm::Collection::getSize())>(
                            read_message_size.data()
                        )
                    };

                    read_message_data.resize(message_size);

                    boost::asio::async_read(
                        socket,
                        boost::asio::buffer(&read_message_data, message_size),
                        [&, this](const boost::system::error_code error, const std::size_t bytes_count)
                        {
                            if(listening_enabled.load())
                            {
                                message_callbacks[read_message_data.retrieve<MessageIdEnum>()](
                                    read_message_data, *this
                                );
                            }
                            else
                            {
                                // Message shouldn't be processed
                            }
                        }
                    )
                }
                else
                {
                    // Message shouldn't be read
                }
            }
        )
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::setOnReceiving(const MessageIdEnum message_id, const MessageReceivedCallback& callback)
    {
        message_callbacks[message_id] = callback;
    }
}