#include <iostream>
#include <thread>
#include <future>

// Function to be executed by the worker thread
void producer(std::promise<int>&& promiseObj) {
    std::cout << "Producer thread launched" << std::endl;
    // Perform some computation or task
    int result = 42;  // Example result
    // Set the value in the promise
    promiseObj.set_value(result);
}

int main() {
    // Create a promise object
    std::promise<int> promiseObj;
    // Obtain a future from the promise
    std::future<int> futureObj = promiseObj.get_future();

    // Launch the worker thread, passing the promise
    std::thread worker(producer, std::move(promiseObj));

    // Wait for the result and get it from the future
    int result = futureObj.get();
    std::cout << "Result obtained from the future: " << result << std::endl;

    // Join the worker thread
    worker.join();

    return 0;
}
