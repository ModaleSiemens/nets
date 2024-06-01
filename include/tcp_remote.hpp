#pragma once

#include <expected>
#include <chrono>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <thread>

#include "types.hpp"
#include "../subprojects/collection/include/collection.hpp"

namespace nets
{
    template <typename MessageIdEnum>
    class TcpRemote
    {
        public:
            using PingTime = std::chrono::duration<double>;
            using MessageReceivedCallback = std::function<void(const mdsm::Collection& collection, TcpRemote& remote)>;

            //TcpRemote(nets::TcpSocket& socket);
            TcpRemote(
                boost::asio::io_context& io_context,
                const PingTime ping_timeout_period,
                const PingTime ping_delay
            );

            void enablePinging          (const bool value = true);
            void enableBeingPinged      (const bool value = true);
            void enableReceivingMessages(const bool value = true);

            ~TcpRemote();
            
            virtual void initialize() {}

            void asyncSend(const mdsm::Collection& message);
            void syncSend(const mdsm::Collection& message);

            virtual void onFailedSending(const mdsm::Collection& message) {};

            void setOnReceiving(
                const MessageIdEnum message_id,
                const MessageReceivedCallback& callback,
                const bool enabled = true
            );

            void setPingingTimeoutPeriod(const PingTime period);

            virtual void onPingingTimeout()   {};
            virtual void onPingFailedSending() {};

            std::expected<PingTime, nets::PingError> ping(const PingTime period = PingTime{0});

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

            std::atomic_bool pinging_enabled            {false};
            std::atomic_bool receiving_messages_enabled {true};

            PingTime ping_timeout_period;
            PingTime ping_delay;

            std::unordered_map<MessageIdEnum, std::pair<MessageReceivedCallback, bool>> message_callbacks;

            std::vector<std::byte> read_message_size;
            mdsm::Collection       read_message_data;

            std::atomic_bool ping_response_received;

            std::atomic_bool active         {true};
            std::atomic_bool socket_is_open {false};

            void startPinging();
            void stopPinging();     

            void startListeningForIncomingMessages();
            void stopListeningForIncomingMessages();           

            void startListeningForPings();
            void stopListeningForPings();                    
    };
}

// Implementation

namespace nets
{
    template <typename MessageIdEnum>
    TcpRemote<MessageIdEnum>::TcpRemote(
        boost::asio::io_context& io_context,
        const PingTime ping_timeout_period,
        const PingTime ping_delay
    )
    :
        io_context{io_context},
        socket{io_context},
        ping_timeout_period{ping_timeout_period},
        ping_delay{ping_delay}
    {
        read_message_size.resize(sizeof(mdsm::Collection::Size));

        message_callbacks[MessageIdEnum::ping_response].first = [&, this](
                const mdsm::Collection& collection,
                TcpRemote<MessageIdEnum>& remote
            )
            {
                ping_response_received = true;               
            }
        ;

        message_callbacks[MessageIdEnum::ping_request].first = [&, this](
                const mdsm::Collection& collection,
                TcpRemote<MessageIdEnum>& remote
            )
            {
                asyncSend(mdsm::Collection{} << MessageIdEnum::ping_response);               
            }
        ;  

        startListeningForPings();

        initialize();

        std::thread {
            [&, this]
            {   
                while(active)
                {
                    if(socket_is_open.load() != socket.is_open())
                    {
                        socket_is_open = socket.is_open();

                        if(socket_is_open)
                        {
                            if(pinging_enabled.load())
                            {
                                startPinging();
                            }

                            if(receiving_messages_enabled.load())
                            {
                                startListeningForIncomingMessages();
                            }
                        }                     
                    }
                }
            }
        }.detach();
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::enablePinging(const bool value)
    {
        pinging_enabled = value;
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::enableBeingPinged(const bool value)
    {
        message_callbacks[MessageIdEnum::ping_request].second = value;
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::enableReceivingMessages(const bool value)
    {
        receiving_messages_enabled = value;
    }

    template <typename MessageIdEnum>
    TcpRemote<MessageIdEnum>::~TcpRemote()
    {
        active = false;
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::stopListeningForPings()
    {
        message_callbacks[MessageIdEnum::ping_request].second = false;
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::startListeningForPings()
    {
        message_callbacks[MessageIdEnum::ping_request].second = true;    
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
        try
        {
            syncSend(mdsm::Collection{} << MessageIdEnum::probe);

            return true;
        }
        catch(boost::exception& e)
        {
            return false;
        }
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
            boost::asio::buffer(message_with_header.data(), message_with_header.size()),
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
    void TcpRemote<MessageIdEnum>::syncSend(const mdsm::Collection& message)
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

        boost::asio::write(
            socket,
            boost::asio::buffer(message_with_header.data(), message_with_header.size())
        );
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::startListeningForIncomingMessages()
    {
        receiving_messages_enabled =  true;

        boost::asio::async_read(
            socket,
            boost::asio::buffer(&read_message_size, sizeof(read_message_size)),
            [&, this](const boost::system::error_code error, const std::size_t bytes_count)
            {
                if(receiving_messages_enabled.load() && socket_is_open.load())
                {
                    const auto message_size {
                        mdsm::Collection::prepareDataForExtracting<mdsm::Collection::Size>(
                            read_message_size.data()
                        )
                    };

                    read_message_data.resize(message_size);

                    boost::asio::async_read(
                        socket,
                        boost::asio::buffer(&read_message_data, message_size),
                        [&, this](const boost::system::error_code error, const std::size_t bytes_count)
                        {
                            if(receiving_messages_enabled.load() && socket_is_open.load())
                            {
                                const auto message_id {
                                    read_message_data.retrieve<MessageIdEnum>()
                                };

                                if(message_callbacks.contains(message_id))
                                {
                                    if(message_callbacks[message_id].second)
                                    {
                                        message_callbacks[message_id].first(
                                            read_message_data, *this
                                        );
                                    }
                                    else
                                    {
                                        // Callback is disabled
                                    }
                                }
                                else 
                                {
                                    // No callback found for received message id
                                }

                                startListeningForIncomingMessages();
                            }
                            else
                            {
                                // Message shouldn't be processed
                            }
                        }
                    );
                }
                else
                {
                    // Message shouldn't be read
                }
            }
        );
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::stopListeningForIncomingMessages()
    {
        receiving_messages_enabled = false;
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::startPinging()
    {
        pinging_enabled = true;

        std::thread {
            [&, this]
            {
                while(pinging_enabled.load() && socket_is_open.load())
                {
                    const auto pinging_result {ping()};

                    if(!pinging_result.has_value())
                    {
                        if(pinging_result.error() == PingError::expired)
                        {
                            onPingingTimeout();
                        }
                        else 
                        {
                            onPingFailedSending();
                        }
                    }

                    std::this_thread::sleep_for(
                        ping_delay - (pinging_result.has_value() ? pinging_result.value() : PingTime{0})
                    );
                }
            }
        }.detach();
    }

    template <typename MessageIdEnum>
    std::expected<typename TcpRemote<MessageIdEnum>::PingTime, nets::PingError>
        TcpRemote<MessageIdEnum>::ping(const PingTime period)
    {
        try
        {
            syncSend(mdsm::Collection{} << MessageIdEnum::ping_request);

            const auto ping_sent_time {std::chrono::system_clock::now()};

            while(!ping_response_received)
            {
                if((ping_sent_time + ping_timeout_period) >= std::chrono::system_clock::now())
                {
                    return std::unexpected(PingError::expired);
                }
            }

            ping_response_received = false;

            return std::chrono::system_clock::now() - ping_sent_time;
        }
        catch(const boost::exception& e)
        {
            return std::unexpected(PingError::failed_to_send);
        }
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::stopPinging()
    {
        pinging_enabled = false;
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::setOnReceiving(
        const MessageIdEnum message_id,
        const MessageReceivedCallback& callback,
        const bool enabled
    )
    {
        message_callbacks[message_id].first = callback;
        message_callbacks[message_id].second = enabled;
    }
}