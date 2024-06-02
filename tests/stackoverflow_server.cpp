#include <boost/asio.hpp>
#include <print>
#include <functional>

using namespace boost::asio;

class Server
{
    public:
        Server()
        :
            socket{io_c},
            acceptor{
                io_c, ip::tcp::endpoint
                {
                    ip::tcp::v4(),
                    60001
                }
            },
            work{io_c.get_executor()}
        {
            std::thread {
                [&, this]
                {
                    io_c.run();
                }
            }.detach();
        }

        void listen()
        {
            std::vector<int> buff {}; 

            buff.resize(8);

            async_read(
                socket,
                boost::asio::buffer(buff),
                [&, this](boost::system::error_code error, std::size_t count)
                {
                    std::println("Inside read callback");
                } 
            );
        }

        void accept()
        {
            acceptor.async_accept(
                make_strand(acceptor.get_executor()),
                std::bind(
                    &Server::handle_accepting,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
            );
        }

        void handle_accepting(boost::system::error_code error, ip::tcp::socket s)
        {
            accept();

            send();

            listen();
        }

        void send()
        {
            std::println("Inside send()");

            post(
                socket.get_executor(),
                std::bind(
                    &Server::send_handle,
                    this
                )
            );
        }

        void send_handle()
        {
            std::println("Inside send_handle()");
        }

    private:
        io_context io_c;
        ip::tcp::acceptor acceptor;
        ip::tcp::socket   socket;
        boost::asio::executor_work_guard<decltype(io_c.get_executor())> work;
};

int main()
{
    Server server;

    server.accept();

    while(true)
    {
    }

    return 0;
}  