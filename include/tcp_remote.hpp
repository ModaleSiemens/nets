#pragma once

#include <expected>
#include <chrono>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <thread>
#include <memory>
#include <deque>
#include <optional>

#include <print>

#include "types.hpp"
#include "collection.hpp"

namespace nets
{
    template <typename MessageIdEnum>
    class TcpRemote : public std::enable_shared_from_this<TcpRemote<MessageIdEnum>>
    {
        public:
            using PingTime = std::chrono::duration<double>;
            using MessageReceivedCallback = std::function<void(mdsm::Collection collection, TcpRemote& remote)>;

            //TcpRemote(nets::TcpSocket& socket);
            TcpRemote(
                boost::asio::io_context& io_context,
                const PingTime           ping_timeout_period,
                const PingTime           ping_delay
            );

            void start();
            void stop();

            bool isConnected();

            ~TcpRemote();

            void send(const mdsm::Collection& message);
            //void syncSend(const mdsm::Collection& message);

            virtual void onFailedSending (mdsm::Collection message) {};
            virtual void onFailedReading (
                std::optional<boost::system::error_code> error = std::nullopt
            )
            {
            };
            
            virtual void onPingingTimeout() {};


            void setOnReceiving(
                const MessageIdEnum message_id,
                const MessageReceivedCallback& callback,
                const bool enabled = true
            );

            void setPingingTimeoutPeriod(const PingTime period);

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

            PingTime ping_timeout_period;
            PingTime ping_delay;

            std::unordered_map<MessageIdEnum, std::pair<MessageReceivedCallback, bool>> message_callbacks;

            std::vector<std::byte> read_message_size;
            mdsm::Collection       read_message_data;

            std::atomic_bool ping_response_received {false};

            std::atomic_bool active       {true};
            std::atomic_bool is_connected {false};

            std::deque<mdsm::Collection> outgoing_messages_queue;        

            bool connectionIsOpen();    

            void asyncSend(const mdsm::Collection& message);
            void messagesSenderLoop();     
            void sendMessageToQueue(const mdsm::Collection& message);

            void startPinging();     

            void startMessagesListener();  
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
        io_context               {io_context},
        socket                   {io_context},
        ping_timeout_period      {ping_timeout_period},
        ping_delay               {ping_delay}
    {
        message_callbacks[MessageIdEnum::ping_request].second  = true;
        message_callbacks[MessageIdEnum::ping_response].second = true;

        read_message_size.resize(sizeof(mdsm::Collection::Size));

        message_callbacks[MessageIdEnum::ping_response].first = [&, this](
                const mdsm::Collection& collection,
                TcpRemote<MessageIdEnum>& remote
            )
            {
                //std::println("DEBUG: Received ping response");

                ping_response_received = true;            
                //is_connected           = true;   
            }
        ;

        message_callbacks[MessageIdEnum::ping_request].first = [&, this](
                const mdsm::Collection& collection,
                TcpRemote<MessageIdEnum>& remote
            )
            {
                //std::println("DEBUG: Receiving ping request");

                send(mdsm::Collection{} << MessageIdEnum::ping_response);               
            }
        ;
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::start()
    {
        is_connected = true;

        startPinging();

        startMessagesListener();
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::stop()
    {
        is_connected = false;
    }

    template <typename MessageIdEnum>
    bool TcpRemote<MessageIdEnum>::isConnected()
    {
        return is_connected.load();
    }

    template <typename MessageIdEnum>
    TcpRemote<MessageIdEnum>::~TcpRemote()
    {
        active = false;
        is_connected = false;
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::send(const mdsm::Collection &message)
    {
        //std::println("DEBUG: send() start");

        boost::asio::post(
            socket.get_executor(),
            std::bind(
                &TcpRemote<MessageIdEnum>::sendMessageToQueue,
                this,
                message
            )
        );
        
        //std::println("DEBUG: send() end");
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
        //std::println("DEBUG: Sending message");

        const auto message_size {message.getSize()};

        //std::println("DEBUG: Header size: {}, Body size: {}", sizeof(message_size), message_size);

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
                if(!error)
                {
                    outgoing_messages_queue.pop_front();
                    
                    messagesSenderLoop();
                }
                else 
                {
                    std::thread {
                        onFailedSending, this, message
                    }.detach();
                }
            }
        );
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::messagesSenderLoop()
    {
        if(outgoing_messages_queue.empty())
        {
            return;
        }

        asyncSend(outgoing_messages_queue.front());
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::sendMessageToQueue(const mdsm::Collection &message)
    {
        //std::println("DEBUG: Sending message to queue");

        outgoing_messages_queue.push_back(message);

        if(outgoing_messages_queue.size() == 1)
        {
            messagesSenderLoop();
        };
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::startMessagesListener()
    {
        if(!is_connected.load())
        {
            //std::println("DEBUG: Not continuing startMessageListener()");
            return;
        }

        boost::asio::async_read(
            socket,
            boost::asio::buffer(read_message_size.data(), read_message_size.size()),
            [&, this](const boost::system::error_code error, const std::size_t bytes_count)
            {
                if(error)
                {
                    is_connected = false;

                    std::thread {
                        onFailedReading, this, error
                    }.detach();

                    return;
                }

                //std::println("DEBUG: Inside first async read callback");

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
                        if(error)
                        {
                            is_connected = false;

                            std::thread {
                                onFailedReading, this, error
                            }.detach();

                            return;
                        }

                        //std::println("DEBUG: Reading message body");

                        const auto message_id {
                            read_message_data.retrieve<MessageIdEnum>()
                        };

                        if(message_callbacks.contains(message_id))
                        {
                            if(message_callbacks[message_id].second)
                            {
                                //std::println("DEBUG: Callback is being called - Message ID = {}", static_cast<std::size_t>(message_id));

                                std::thread {
                                    message_callbacks[message_id].first,
                                    read_message_data,
                                    std::ref(*this)
                                }.detach();
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

                        startMessagesListener();
                    }

                );
            }
        );
    }

    template <typename MessageIdEnum>
    void TcpRemote<MessageIdEnum>::startPinging()
    {
        std::thread {
            [&, this]
            {
                while(is_connected)
                {
                    //std::println("DEBUG: Pinging");

                    const auto pinging_result {ping()};

                    if(!pinging_result.has_value())
                    {
                        //std::println("DEBUG: pinging_result doesn't have value");

                        if(pinging_result.error() == PingError::expired)
                        {
                            //std::println("DEBUG: Pinging timeout");

                            is_connected = false;

                            onPingingTimeout();
                        }
                        else 
                        {
                            //std::println("DEBUG: Failed sending ping");

                            is_connected = false;

                            onFailedReading();
                        }
                    }
                    else 
                    {
                        // Ping response received successfully
                        
                        //std::println("DEBUG: Pinging succeeded");

                        is_connected = true;
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
            send(mdsm::Collection{} << MessageIdEnum::ping_request);

            const auto ping_sent_time {std::chrono::system_clock::now()};

            while(!ping_response_received.load())
            {
                if((ping_sent_time + ping_timeout_period) <= std::chrono::system_clock::now())
                {
                    //std::println("DEBUG: Inside Ping Expired if");

                    ping_response_received = false;

                    return std::unexpected(PingError::expired);
                }
            }

            //std::println("DEBUG: Exited ping while loop");

            return std::chrono::system_clock::now() - ping_sent_time;
        }
        catch(const boost::exception& e)
        {
            ping_response_received = false;

            return std::unexpected(PingError::failed_to_send);
        }
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