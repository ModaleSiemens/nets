#include <boost/asio.hpp>
#include <print>
#include <functional>

using namespace boost::asio;

class Client
{
    public:
        Client()
        :
            socket{io_c},
            work{io_c.get_executor()}
        {
            std::thread {
                [&, this]
                {
                    io_c.run();
                }
            }.detach();
        }

        void connect()
        {
            boost::system::error_code error;

            ip::tcp::resolver resolver {io_c};

            boost::asio::connect(
                socket,
                resolver.resolve("localhost", "60001"),
                error
            );

            if(error)
            {
                std::println("Error connecting");
            }
        }

    private:
        io_context io_c;
        ip::tcp::socket socket;
        boost::asio::executor_work_guard<decltype(io_c.get_executor())> work;
};

int main()
{
    Client client;

    client.connect();

    while(true)
    {
    }

    return 0;
}