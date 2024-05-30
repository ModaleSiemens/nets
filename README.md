## Pseudo-Code

### TCP Client

- Active sockets of different types (IPv4, IPv6) can be represented by the same class, so there's no need to differentiate them in the constructor.
- Address may be a DNS name or a raw address: it's type will be indicated by the enum class Address Kind.

```cpp
class TcpClient
{
    public:
        TcpClient(
            const std::string_view address,
            const nets::Port port,
            const nets::AddressKind address_kind
        );

        void connect()    const;
        void disconnect() const;

        virtual void onConnection()    const;
        virtual void onDisconnection() const;

        std::string_view getServerAddress() const;
        nets::Port       getServerPort()    const;

        ~TcpClient();
};
```
### TCP Server
```cpp
class TcpServer
{
    public:
        TcpServer(
            const nets::Port port,
            const nets::IPVersion ip_version,
            const std::string_view address = "" // Specific host address may be specified
        );

        void start() const;
        void stop () const;

        virtual void onClientConnection   (      nets::Remote& remote) const;
        virtual void onClientDisconnection(const nets::Remote& remote) const;

        void closeConnection(nets::Remote& remote);
        void closeAllConnections();

        size_t getClientsCount() const;

              std::vector<Remote>& getClients();
        const std::vector<Remote>& getClients() const;

        ~TcpServer();
};
```
### Sender
```cpp
class Sender
{
    public: 
        Sender(Socket& socket);

        void send(const mdsm::Collection message); // throws if couldn't send/receive

        template <typename T>
        T receive(const mdsm::Collection message); // throws if couldn't send/receive
        
        template <typename T>                      // throws if couldn't receive
        T receive();
};
```