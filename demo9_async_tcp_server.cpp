#include <boost/asio.hpp> // Include Boost.Asio for networking
#include <iostream> // Include iostream for console I/O
#include <memory> // Include memory for std::shared_ptr

using boost::asio::ip::tcp; // Using TCP from Boost.Asio's IP namespace

// Session class to handle individual client connections
class Session : public std::enable_shared_from_this<Session> {
public:
    // Constructor initializes the socket with a moved socket
    Session(tcp::socket socket)
        : socket_(std::move(socket)) {}

    // Start the session by initiating an asynchronous read
    void start() {
        async_read();
    }

private:
    // Asynchronously read data from the client
    void async_read() {
        auto self(shared_from_this()); // Keep a shared pointer to this instance
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) { // If no error occurred
                    std::cout << "Received: " << std::string(data_, length) << std::endl;
                    async_write(); // Write a response after reading data
                }
            });
    }

    // Asynchronously write a response to the client
    void async_write() {
        auto self(shared_from_this()); // Keep a shared pointer to this instance
        const std::string msg = "Hello from server!"; // Response message
        boost::asio::async_write(socket_, boost::asio::buffer(msg),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) { // If no error occurred
                    // Close the socket after sending the response
                    socket_.shutdown(tcp::socket::shutdown_both, ec);
                    socket_.close(ec);
                }
            });
    }

    tcp::socket socket_; // Socket for communication with the client
    enum { max_length = 1024 }; // Maximum length of data to read
    char data_[max_length]; // Data buffer
};

// TcpServer class to accept incoming client connections
class TcpServer {
public:
    // Constructor initializes the acceptor and starts accepting connections
    TcpServer(boost::asio::io_context& io_context, short port)
        : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        start_accept(); // Start accepting connections
    }

private:
    // Start accepting new client connections
    void start_accept() {
        // Create a new socket for the next connection
        auto new_session = std::make_shared<tcp::socket>(io_context_);
        // Asynchronously accept a new connection
        acceptor_.async_accept(*new_session,
            [this, new_session](const boost::system::error_code& error) {
                if (!error) { // If no error occurred
                    // Start a new session for the accepted connection
                    std::make_shared<Session>(std::move(*new_session))->start();
                }
                start_accept(); // Continue accepting connections
            });
    }

    boost::asio::io_context& io_context_; // Reference to the IO context
    tcp::acceptor acceptor_; // Acceptor to listen for incoming connections
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) { // Check if the port number is provided
            std::cerr << "Usage: TcpServer <port>\n";
            return 1;
        }

        boost::asio::io_context io_context; // Create an IO context
        TcpServer server(io_context, std::atoi(argv[1])); // Create a server with the specified port
        io_context.run(); // Run the IO context to start handling events
    } catch (std::exception& e) { // Catch any exceptions
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
