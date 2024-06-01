#pragma once

#include <expected>
#include <chrono>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <thread>
#include <memory>
#include <mutex>

#include <print>

#include "types.hpp"
#include "../subprojects/collection/include/collection.hpp"

namespace nets
{
    template <typename MessageIdEnum>
    class TcpRemote : public std::enable_shared_from_this<TcpRemote<MessageIdEnum>>
    {
        public:
            using PingTime = std::chrono::duration<double>;
            using MessageReceivedCallback = std::function<void(const mdsm::Collection& collection, TcpRemote& remote)>;

            //TcpRemote(nets::TcpSocket& socket);
            TcpRemote(
                boost::asio::io_context& io_context,
                const PingTime           ping_timeout_period,
                const PingTime           ping_delay,
                const bool               enable_pinging = true,
                const bool               enable_being_pinged = true,
                const bool               enable_receiving_messages = true
            );

            void enablePinging          (const bool value = true);
            void enableBeingPinged      (const bool value = true);
            void enableReceivingMessages(const bool value = true);

            bool isConnected();

            ~TcpRemote();

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

            nets::TcpSocket& getSocket();

            bool operator==(const TcpRemote& remote);

        private:
            boost::asio::io_context& io_context;
            nets::TcpSocket          socket;

            std::string address;
            nets::Port  port;

            std::atomic_bool pinging_enabled;
            std::atomic_bool receiving_messages_enabled;

            PingTime ping_timeout_period;
            PingTime ping_delay;

            std::unordered_map<MessageIdEnum, std::pair<MessageReceivedCallback, bool>> message_callbacks;

            std::vector<std::byte> read_message_size;
            mdsm::Collection       read_message_data;

            std::atomic_bool ping_response_received;

            std::atomic_bool active         {true};
            std::atomic_bool socket_is_open {false};

            std::mutex mutex;

            void startPinging();
            void stopPinging();     

            void startListeningForIncomingMessages();
            void stopListeningForIncomingMessages();           

            void startListeningForPings();
            void stopListeningForPings();           

            bool connectionIsOpen();         
    };
}

// Implementation

namespace nets
{
    template <typename MessageIdEnum>
    TcpRemote<MessageIdEnum>::TcpRemote(
        boost::asio::io_context& io_context,
        const PingTime ping_timeout_period,
        const PingTime ping_delay,
        const bool enable_pinging,
        const bool enable_being_pinged,
        const bool enable_receiving_messages
    )
    :
        io_context               {io_context},
        socket                   {io_context},
        ping_timeout_period      {ping_timeout_period},
        ping_delay               {ping_delay}
    {
        pinging_enabled = enable_pinging;
        message_callbacks[MessageIdEnum::ping_request].second  = enable_being_pinged;
        message_callbacks[MessageIdEnum::ping_response].second = enable_pinging;
        receiving_messages_enabled = enable_receiving_messages;


        read_message_size.resize(sizeof(mdsm::Collection::Size));

        message_callbacks[MessageIdEnum::ping_response].first = [&, this](
                const mdsm::Collection& collection,
                TcpRemote<MessageIdEnum>& remote
            )
            {
                std::println("DEBUG: Received ping response");

                ping_response_received = true;               
            }
        ;

        message_callbacks[MessageIdEnum::ping_request].first = [&, this](
                const mdsm::Collection& collection,
                TcpRemote<MessageIdEnum>& remote
            )
            {
                std::println("DEBUG: Receiving ping request");

                asyncSend(mdsm::Collection{} << MessageIdEnum::ping_response);               
            }
        ;  

        std::thread {
            [&, this]
            {   
                while(active)
                {
                    if(socket_is_open.load() != connectionIsOpen())
                    {
                        socket_is_open = connectionIsOpen();

                        std::println("DEBUG: Socket is open: {}", socket_is_open.load());

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

                std::this_thread::sleep_for(std::chrono::milliseconds{100});
            }
        }.detach();        
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::enablePinging(const bool value)
    {
        message_callbacks[MessageIdEnum::ping_response].second = value;
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
    bool TcpRemote<MessageIdEnum>::isConnected()
    {
        return socket_is_open.load();;
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
        if(!socket_is_open.load())
        {
            return;
        }

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

        std::memcpy(message_with_header.data(), prepared_message_size.data(), prepared_message_size.size());
        std::memcpy(
            message_with_header.data() + prepared_message_size.size(),
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
        if(!socket_is_open.load())
            return;

        boost::asio::async_read(
            socket,
            boost::asio::buffer(read_message_size.data(), read_message_size.size()),
            [&, this](const boost::system::error_code error, const std::size_t bytes_count)
            {
                if(receiving_messages_enabled.load())
                {
                    //std::println("DEBUG: Reading message size");

                    const auto message_size {
                        mdsm::Collection::prepareDataForExtracting<mdsm::Collection::Size>(
                            read_message_size.data()
                        )
                    };

                    read_message_data.resize(message_size);

                    boost::asio::async_read(
                        socket,
                        boost::asio::buffer(read_message_data.getData(), message_size),
                        [&, this](const boost::system::error_code error, const std::size_t bytes_count)
                        {
                            if(receiving_messages_enabled.load())
                            {
                                //std::println("DEBUG: Reading message body");

                                const auto message_id {
                                    read_message_data.retrieve<MessageIdEnum>()
                                };

                                if(message_callbacks.contains(message_id))
                                {
                                    if(message_callbacks[message_id].second)
                                    {
                                        std::println("DEBUG: Callback is being called - Message ID = {}", static_cast<std::size_t>(message_id));

                                        message_callbacks[message_id].first(
                                            read_message_data, std::ref(*this)
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
        if(!socket_is_open.load())
            return;

        std::thread {
            [&, this]
            {
                while(pinging_enabled.load() && socket_is_open.load())
                {
                    std::println("DEBUG: Pinging");

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

            while(!ping_response_received.load())
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