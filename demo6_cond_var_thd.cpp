#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

// Shared resources
std::queue<int> dataQueue;
std::mutex mtx;
std::condition_variable cv;
const int maxQueueSize = 10;

// Producer function
void producer() {
    for (int i = 0; i < 20; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{ return dataQueue.size() < maxQueueSize; }); // Wait until there's space in the queue
        dataQueue.push(i);
        std::cout << "Produced: " << i << std::endl;
        lock.unlock();
        cv.notify_all(); // Notify the consumer that data is available
    }
}

// Consumer function
void consumer() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{ return !dataQueue.empty(); }); // Wait until there's data in the queue
        if (dataQueue.empty()) {
            break; // Exit the loop if the queue is empty and no more items are produced
        }
        int data = dataQueue.front();
        dataQueue.pop();
        std::cout << "Consumed: " << data << std::endl;
        lock.unlock();
        cv.notify_all(); // Notify the producer that there's space in the queue
    }
}

int main() {
    std::thread producerThread(producer);
    std::thread consumerThread(consumer);

    producerThread.join();
    consumerThread.join();

    return 0;
}
