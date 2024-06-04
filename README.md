# ModaleSiemens::nets

This is an object-oriented, asyncronous Boost.Asio-based C++ networking header-only library. 

This library contains a TcpClient class, a TcpServer class and a TcpRemote (represents a connection) class.



## Echo Client-Server example

### `MessageIds` enum class
```cpp
enum class MessageIds
{
    ping_request, ping_response,
    message_request,
    message_response
};
```
### `Remote` class
```cpp
class Remote : public nets::TcpRemote<MessageIds>
{
    public:
        using TcpRemote<MessageIds>::TcpRemote;

        void onFailedSending(mdsm::Collection message) override
        {
            std::println("Failed to send message...");
        }

        void onFailedReading(std::optional<boost::system::error_code> error) override
        {
            std::println("Failed to read from remote...");
        }

        void onPingingTimeout() override
        {
            std::println("Remote didn't respond in time...");
        }
};
```
The same `nets::TcpRemote<>` template class derived class `Remote` is used for both the client and the server.

It's instantiated with `MessageIds` as template type parameter. This enum class is used for exchanging messages between two remotes, and **must** contain at least two enumerators: `ping_request` and `ping_response`, used internally in pinging operations.

`Remote` overrides the three base class virtual (not pure) functions:
```cpp
void onFailedSending (mdsm::Collection message)
void onFailedReading (std::optional<boost::system::error_code> error)
void onPingingTimeout()
```
These functions are called when:
1. The remote fails sending data;
2. The remote fails reading data;
3. The other machine doesn't respond to a pinging in time;

### Echo `Client` class

```cpp
class Client : public nets::TcpClient<MessageIds, Remote>
{
    public:
        using TcpClient<MessageIds, Remote>::TcpClient;

        virtual void onConnection(std::shared_ptr<Remote> server) override
        {
            using namespace mdsm;

            server->setOnReceiving(
                MessageIds::message_response,
                [&, this](Collection message, nets::TcpRemote<MessageIds>& server)
                {
                    std::println("{}", message.retrieve<std::string>());
                }
            );

            while(server->isConnected())
            {
                std::string message_to_be_sent;

                std::getline(std::cin, message_to_be_sent);

                server->send(
                    Collection{} << MessageIds::message_request << message_to_be_sent
                );
            }

            std::println("Server closed connection...");

            disconnect();
        }
};

```