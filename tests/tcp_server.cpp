#include <boost/asio.hpp>

using namespace std;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
    public:
        using Pointer = std::shared_ptr<TcpConnection>;

        static Pointer getShared(boost::asio::io_context& io_context)
        {
            return std::make_shared<TcpConnection>(TcpConnection(io_context));
        }

        boost::asio::ip::tcp::socket& getSocket()
        {
            return socket;
        }

        void start()

    private:
        TcpConnection(boost::asio::io_context& io_context)
        :   
            socket{io_context}
        {
        }

        boost::asio::ip::tcp::socket socket;
        std::string message;
};

class TcpServer
{
    public:
        TcpServer(const boost::asio::ip::port_type port)
        :
            io_context{},
            acceptor(
                io_context,
                boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)
            )
        {
        }

        void start()
        {
            
        }

    private:
        boost::asio::io_context        io_context;
        boost::asio::ip::tcp::acceptor acceptor;
        
};

int main()
{

}