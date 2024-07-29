#include <iostream>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <queue>

// Shared resources
std::queue<int> dataQueue;
std::mutex mtx;
std::condition_variable cv;
const int maxQueueSize = 10;
bool done = false; // Flag to signal termination

// Producer function
std::future<void> producer() {
    return std::async(std::launch::async, [] {
        for (int i = 0; i < 20; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, []{ return dataQueue.size() < maxQueueSize; }); // Wait until there's space in the queue
            dataQueue.push(i);
            std::cout << "Produced: " << i << std::endl;
            lock.unlock();
            cv.notify_all(); // Notify the consumer that data is available
        }

        // Notify the consumer that production is done
        std::unique_lock<std::mutex> lock(mtx);
        done = true;
        lock.unlock();
        cv.notify_all(); // Notify the consumer to exit the loop
    });
}

// Consumer function
std::future<void> consumer() {
    return std::async(std::launch::async, [] {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, []{ return !dataQueue.empty() || done; }); // Wait until there's data or production is done
            if (dataQueue.empty() && done) {
                break; // Exit the loop if the queue is empty and production is done
            }
            if (!dataQueue.empty()) {
                int data = dataQueue.front();
                dataQueue.pop();
                std::cout << "Consumed: " << data << std::endl;
            }
            lock.unlock();
            cv.notify_all(); // Notify the producer that there's space in the queue
        }
    });
}

int main() {
    // Launch producer and consumer asynchronously
    std::future<void> producerFuture = producer();
    std::future<void> consumerFuture = consumer();

    // Wait for both tasks to complete
    producerFuture.get();
    consumerFuture.get();

    return 0;
}
