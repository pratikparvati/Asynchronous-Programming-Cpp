#include <aio.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <csignal>
#include <mutex>
#include <condition_variable>

const int BUFFER_SIZE = 1024;  // Size of the buffer for asynchronous operations

// Function to handle errors by printing a message and exiting
void handle_error(const char* msg) {
    perror(msg);  // Print the error message
    exit(EXIT_FAILURE);  // Exit the program with a failure status
}

// Structure to hold data for asynchronous read and write operations
struct aio_data {
    aiocb aiocb_read;  // Control block for the asynchronous read operation
    aiocb aiocb_write;  // Control block for the asynchronous write operation
    int output_fd;  // File descriptor for the output file
    off_t offset;  // Current offset for reading from the input file
};

// Condition variable and mutex to signal the completion of asynchronous operations
std::condition_variable cv;
std::mutex cv_m;
bool done = false;  // Flag to indicate if the operations are complete

// Callback function for handling the completion of asynchronous read and write operations
void aio_completion_handler(sigval_t sigval) {
    auto data = static_cast<aio_data*>(sigval.sival_ptr);
    auto aiocb_read = &(data->aiocb_read);
    auto aiocb_write = &(data->aiocb_write);

    int err = aio_error(aiocb_read);  // Check the status of the read operation
    if (err == 0) {
        int bytes_read = aio_return(aiocb_read);  // Get the number of bytes read

        if (bytes_read > 0) {
            // Update the offset for the next read operation
            data->offset += bytes_read;

            // Set up the control block for the write operation
            memset(aiocb_write, 0, sizeof(struct aiocb));
            aiocb_write->aio_fildes = data->output_fd;
            aiocb_write->aio_buf = aiocb_read->aio_buf;
            aiocb_write->aio_nbytes = bytes_read;
            aiocb_write->aio_offset = aiocb_read->aio_offset;
            aiocb_write->aio_sigevent.sigev_notify = SIGEV_THREAD;
            aiocb_write->aio_sigevent.sigev_notify_function = aio_completion_handler;
            aiocb_write->aio_sigevent.sigev_notify_attributes = nullptr;
            aiocb_write->aio_sigevent.sigev_value.sival_ptr = data;

            // Start the asynchronous write operation
            if (aio_write(aiocb_write) == -1) {
                handle_error("aio_write");
            }

            // Prepare for the next read operation
            memset(aiocb_read, 0, sizeof(struct aiocb));
            aiocb_read->aio_fildes = aiocb_write->aio_fildes;
            aiocb_read->aio_buf = aiocb_write->aio_buf;
            aiocb_read->aio_nbytes = BUFFER_SIZE;
            aiocb_read->aio_offset = data->offset;
            aiocb_read->aio_sigevent.sigev_notify = SIGEV_THREAD;
            aiocb_read->aio_sigevent.sigev_notify_function = aio_completion_handler;
            aiocb_read->aio_sigevent.sigev_notify_attributes = nullptr;
            aiocb_read->aio_sigevent.sigev_value.sival_ptr = data;

            // Start the next asynchronous read operation
            if (aio_read(aiocb_read) == -1) {
                handle_error("aio_read");
            }
        } else {
            // If bytes_read is 0, we've reached the end of the file
            close(aiocb_read->aio_fildes);  // Close the input file descriptor
            close(data->output_fd);  // Close the output file descriptor
            delete[] static_cast<char*>(const_cast<void*>(aiocb_read->aio_buf));  // Free the buffer memory
            delete data;  // Delete the aio_data structure
            // Signal the main thread to stop waiting
            {
                std::lock_guard<std::mutex> lock(cv_m);
                done = true;
            }
            cv.notify_one();
        }
    } else {
        // Handle errors in the read operation
        std::cerr << "aio_read error: " << strerror(err) << std::endl;
        handle_error("aio_read");
    }
}

// Function to start the asynchronous read operation
void async_read(const char* input_file, const char* output_file) {
    // Open the input file for reading
    int input_fd = open(input_file, O_RDONLY);
    if (input_fd == -1) {
        handle_error("open input_file");
    }

    // Open the output file for writing, creating it if necessary
    int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd == -1) {
        handle_error("open output_file");
    }

    // Initialize the aio_data structure
    auto data = new aio_data;
    memset(&(data->aiocb_read), 0, sizeof(struct aiocb));
    data->aiocb_read.aio_fildes = input_fd;
    data->aiocb_read.aio_buf = new char[BUFFER_SIZE];
    data->aiocb_read.aio_nbytes = BUFFER_SIZE;
    data->aiocb_read.aio_offset = 0;
    data->aiocb_read.aio_sigevent.sigev_notify = SIGEV_THREAD;
    data->aiocb_read.aio_sigevent.sigev_notify_function = aio_completion_handler;
    data->aiocb_read.aio_sigevent.sigev_notify_attributes = nullptr;
    data->aiocb_read.aio_sigevent.sigev_value.sival_ptr = data;
    data->output_fd = output_fd;
    data->offset = 0;  // Initialize the offset for reading

    // Start the initial asynchronous read operation
    if (aio_read(&(data->aiocb_read)) == -1) {
        handle_error("aio_read");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: AsyncFileIO <input_file> <output_file>\n";
        return 1;
    }

    // Start the asynchronous read operation
    async_read(argv[1], argv[2]);

    // Wait for the asynchronous operations to complete
    std::unique_lock<std::mutex> lk(cv_m);
    cv.wait(lk, []{ return done; });

    return 0;
}
