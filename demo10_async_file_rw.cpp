#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <iostream>
#include <fstream>
#include <memory>

using boost::asio::io_service;
using boost::asio::post;
using namespace boost::asio::ip;

// Handler class to manage asynchronous file operations
class FileHandler : public std::enable_shared_from_this<FileHandler> {
public:
    FileHandler(io_service& io_service, const std::string& input_file, const std::string& output_file)
        : io_service_(io_service),
          input_file_(input_file),
          output_file_(output_file),
          input_stream_(input_file, std::ios::binary),
          output_stream_(output_file, std::ios::binary) {

        if (!input_stream_.is_open()) {
            throw std::runtime_error("Failed to open input file: " + input_file_);
        }

        if (!output_stream_.is_open()) {
            throw std::runtime_error("Failed to open output file: " + output_file_);
        }
    }

    // Start the asynchronous read/write process
    void start() {
        async_read();
    }

private:
    // Asynchronously read data from the input file
    void async_read() {
        auto self(shared_from_this());
        post(io_service_, [this, self]() {
            input_stream_.read(data_, max_length);
            std::size_t length = input_stream_.gcount();

            if (length > 0) {
                async_write(length);
            } else {
                // Close streams when done
                input_stream_.close();
                output_stream_.close();
                std::cout << "File processing completed." << std::endl;
            }
        });
    }

    // Asynchronously write data to the output file
    void async_write(std::size_t length) {
        auto self(shared_from_this());
        post(io_service_, [this, self, length]() {
            output_stream_.write(data_, length);

            if (output_stream_) {
                async_read();
            } else {
                std::cerr << "Error writing to output file." << std::endl;
            }
        });
    }

    io_service& io_service_; // IO service

    std::string input_file_; // Input file name
    std::string output_file_; // Output file name
    std::ifstream input_stream_; // Input file stream
    std::ofstream output_stream_; // Output file stream

    enum { max_length = 1024 }; // Maximum length of data to read/write
    char data_[max_length]; // Data buffer
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: AsyncFileIO <input_file> <output_file>\n";
            return 1;
        }

        io_service io_service;
        std::make_shared<FileHandler>(io_service, argv[1], argv[2])->start();
        io_service.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}

