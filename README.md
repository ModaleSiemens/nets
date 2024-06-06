# ModaleSiemens::nets

This is an object-oriented, asyncronous **Boost.Asio**-based C++ networking header-only library. 

This library contains a `TcpClient` class, a `TcpServer` class and a `TcpRemote` (represents a connection) class; relies on **ModaleSiemens::collection**, a header-only library providing a simple data serializer.

This library uses the **Meson** build system, and exports the dependency `lib_nets_dep`.

## Echo client-server example

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

`Remote` overrides the three base class virtual (not pure) member functions:
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

            disconnect();
        }
};
```
Our custom client class is derived from an instance of the class template `TcpClient`, instantiated with the messages type enum class `MessageIds` and our remote class `Remote`.

`Client` overrides the base class pure virtual member function `onConnection(std::shared_ptr<TcpRemote> server)`, which is called when the client successfully connects to a server.

In this function, we first associate the message type `MessageIds::message_response` with a callback used to print the received string.

Then, we have a while loop which, on each iteration, checks whether the `Remote` is connected (`isConnected()` return value depends on the result of pinging the other machine): inside this while loop, we ask the user for a `\n`-terminated string, which we then send to the server via the non-blocking member function `send()`.

If the while loop ends, it means the connection is closed, so we finally `disconnect()` the client.

### Echo Server class
```cpp
class Server : public nets::TcpServer<MessageIds, Remote>
{
    public:
        using TcpServer<MessageIds, Remote>::TcpServer;

        virtual void onClientConnection(std::shared_ptr<Remote> client) override
        {
            using namespace mdsm;

            client->setOnReceiving(
                MessageIds::message_request,
                [&, this](Collection message, nets::TcpRemote<MessageIds>& server)
                {
                    client->send(
                        Collection{} << MessageIds::message_response << message.retrieve<std::string>()
                    );
                }
            );

            while(client->isConnected())
            {
            }

            closeConnection(client);
        }

        virtual void onForbiddenClientConnection(std::shared_ptr<Remote> client) override
        {
            closeConnection(client);
        }
};
```
As with the `Client` class, our server class is derived from an instance of the class template `TcpServer`, instantiated with the messages type enum class `MessageIds` and our remote class `Remote`.

`Server` ovverides two base class pure virtual member functions:
- `onClientConnection(std::shared_ptr<Remote> client)`, which is called when a client connects normally.
- `onForbiddenClientConnection(std::shared_ptr<Remote> client)`, called when a client connects while the server isn't **theorically** accepting requests.

In the `onClientConnection()` member function, we use the same metod seen in the client class to send back the message we received from the client.

Once the client isn't connected anymore, we officially close the connection.

### Client `main.cpp`

```cpp
int main()
{
    Client client {
        "localhost",
        "60000",
        Remote::PingTime{4},
        Remote::PingTime{6}
    };

    client.connect();

    while(true)
    {
    }

    return 0;
}
```
### Server `main.cpp`

```cpp
int main()
{
    Server server {
        60'000,
        nets::IPVersion::ipv4,
        Remote::PingTime{4},
        Remote::PingTime{6} 
    };

    server.startAccepting();

    while(true)
    {
    }

    return 0;
}

```

`startAccepting()` is a non blocking function, while `connect()` is blocking and returns whether the client successfully connected to the server.


The first `Remote::PingTime` refers to the ping timeout period, while the seconds indicates the delay between consecutive pingings.
