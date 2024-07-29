#include <iostream>
#include <thread>
#include <future>
#include <chrono>

// Function to be executed by the asynchronous task
void producer(std::promise<int> promiseObj) {
    std::cout << "Producer task launched" << std::endl;
    // Simulate some computation or task
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Simulate delay
    int result = 42;  // Example result
    // Set the value in the promise
    promiseObj.set_value(result);
    std::cout << "Producer task running asynchronously" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Simulate work
}

int main() {
    // Create a promise object
    std::promise<int> promiseObj;
    // Obtain a future from the promise
    std::future<int> futureObj = promiseObj.get_future();

    // Launch the asynchronous task, passing the promise
    std::future<void> asyncTask = std::async(std::launch::deferred, producer, std::move(promiseObj));
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Simulate work
    // Simulate a task in the main thread before calling .get()
    std::cout << "Main thread is doing some work..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Simulate work
    asyncTask.get(); // To make sure the async task has completed

    // Wait for the result and get it from the future
    int result = futureObj.get(); // This will block until the producer task is finished
    std::cout << "Result obtained from the future: " << result << std::endl;

    // Ensure the asynchronous task has finished

    return 0;
}
