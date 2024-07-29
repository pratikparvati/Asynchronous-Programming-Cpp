#include <iostream>
#include <thread>
#include <future>
#include <vector>

// Function to be executed by the asynchronous task
int computeValue() {
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Simulate long computation
    return 42; // Example result
}

// Consumer function that reads and prints the value from a shared future
void consumer(std::shared_future<int> sharedFuture) {
    std::cout << "Consumer thread started, waiting for result..." << std::endl;
    int result = sharedFuture.get(); // Retrieve the result
    std::cout << "Consumer thread received result: " << result << std::endl;
}

int main() {
    // Launch asynchronous task and obtain a future
    std::future<int> futureObj = std::async(std::launch::async, computeValue);

    // Convert future to shared_future
    std::shared_future<int> sharedFuture = futureObj.share();

    // Launch multiple consumer threads that use the shared_future
    const int numConsumers = 3;
    std::vector<std::thread> consumers;
    for (int i = 0; i < numConsumers; ++i) {
        consumers.emplace_back(consumer, sharedFuture);
    }

    // Join all consumer threads
    for (auto& th : consumers) {
        th.join();
    }

    return 0;
}
