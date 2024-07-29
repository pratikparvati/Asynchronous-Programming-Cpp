#include <iostream>
#include <thread>
#include <future>
#include <stdexcept>

// Function to be executed by the asynchronous task
void producer(std::promise<int> promiseObj) {
    try {
        // Simulate some computation that throws an exception
        throw std::runtime_error("An error occurred in the asynchronous task.");
    } catch (...) {
        // Set an exception in the promise
        promiseObj.set_exception(std::current_exception());
    }
}

int main() {
    // Create a promise object
    std::promise<int> promiseObj;
    // Obtain a future from the promise
    std::future<int> futureObj = promiseObj.get_future();

    // Launch the asynchronous task, passing the promise
    std::future<void> asyncTask = std::async(std::launch::async, producer, std::move(promiseObj));

    try {
        // Wait for the result and get it from the future
        int result = futureObj.get(); // This will throw an exception if one was set in the promise
        std::cout << "Result obtained from the future: " << result << std::endl;
    } catch (const std::exception& e) {
        // Handle the exception from the asynchronous task
        std::cout << "Caught an exception: " << e.what() << std::endl;
    }

    // Ensure the asynchronous task has finished
    asyncTask.get(); // To make sure the async task has completed

    return 0;
}

