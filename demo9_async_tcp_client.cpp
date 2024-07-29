#include <boost/asio.hpp> // Include Boost.Asio for networking
#include <iostream> // Include iostream for console I/O

using boost::asio::ip::tcp; // Using TCP from Boost.Asio's IP namespace

// TcpClient class to connect to the server and exchange messages
class TcpClient {
public:
    // Constructor initializes the socket and connects to the server
    TcpClient(boost::asio::io_context& io_context, const tcp::resolver::results_type& endpoints)
        : socket_(io_context) {
        connect(endpoints); // Connect to the server
    }

private:
    // Connect to the server asynchronously
    void connect(const tcp::resolver::results_type& endpoints) {
        boost::asio::async_connect(socket_, endpoints,
            [this](const boost::system::error_code& ec, const tcp::endpoint& /*endpoint*/) {
                if (!ec) { // If no error occurred
                    async_write(); // Write a message after connecting
                }
            });
    }

    // Asynchronously read data from the server
    void async_read() {
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
            [this](boost::system::error_code ec, std::size_t length) {
                if (!ec) { // If no error occurred
                    std::cout << "Received: " << std::string(data_, length) << std::endl;
                    // Close the socket after receiving the response
                    socket_.shutdown(tcp::socket::shutdown_both, ec);
                    socket_.close(ec);
                }
            });
    }

    // Asynchronously write a message to the server
    void async_write() {
        const std::string msg = "Hello from client!"; // Message to send
        boost::asio::async_write(socket_, boost::asio::buffer(msg),
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) { // If no error occurred
                    async_read(); // Read the response after sending the message
                }
            });
    }

    tcp::socket socket_; // Socket for communication with the server
    enum { max_length = 1024 }; // Maximum length of data to read
    char data_[max_length]; // Data buffer
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) { // Check if the host and port are provided
            std::cerr << "Usage: TcpClient <host> <port>\n";
            return 1;
        }

        boost::asio::io_context io_context; // Create an IO context
        tcp::resolver resolver(io_context); // Create a resolver to find the server
        auto endpoints = resolver.resolve(argv[1], argv[2]); // Resolve the host and port
        TcpClient client(io_context, endpoints); // Create a client and connect to the server
        io_context.run(); // Run the IO context to start handling events
    } catch (std::exception& e) { // Catch any exceptions
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
