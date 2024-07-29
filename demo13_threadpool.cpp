#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadPool {
public:
    // Constructor: Create a thread pool with a given number of threads
    ThreadPool(size_t threads);
    // Destructor: Clean up all threads
    ~ThreadPool();

    // Add a new task to the pool
    void enqueue(std::function<void()> task);

private:
    // Vector of worker threads
    std::vector<std::thread> workers;
    // Task queue
    std::queue<std::function<void()>> tasks;

    // Synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

    // Mutex for synchronizing console output
    std::mutex print_mutex;
};

// Constructor: Initialize the thread pool
ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this, i] {
            for (;;) {
                std::function<void()> task;

                // Lock the task queue
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });

                    if (this->stop && this->tasks.empty()) return;

                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                } // Release lock

                {
                    // Lock for printing
                    std::lock_guard<std::mutex> lock(this->print_mutex);
                    std::cout << "Thread " << i << " executing task." << std::endl;
                }
                task();
                {
                    // Lock for printing
                    std::lock_guard<std::mutex> lock(this->print_mutex);
                    std::cout << "Thread " << i << " finished task." << std::endl;
                }
            }
        });
    }
}

// Destructor: Join all worker threads
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers) worker.join();
}

// Add a new work item to the pool
void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");
        tasks.emplace(std::move(task));
    }
    condition.notify_one();
}

// Example function to run as a task
void example_function(int n, std::mutex &print_mutex) {
    {
        // Lock for printing
        std::lock_guard<std::mutex> lock(print_mutex);
        std::cout << "Task " << n << " is running." << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Simulate work
    {
        // Lock for printing
        std::lock_guard<std::mutex> lock(print_mutex);
        std::cout << "Task " << n << " is finished." << std::endl;
    }
}

int main() {
    // Create a thread pool with 3 threads
    ThreadPool pool(3);

    // Mutex for synchronizing console output in tasks
    std::mutex print_mutex;

    // Enqueue 5 tasks
    for (int i = 0; i < 5; ++i) {
        pool.enqueue([i, &print_mutex] { example_function(i, print_mutex); });
    }

    // Give some time for tasks to complete
    std::this_thread::sleep_for(std::chrono::seconds(6));

    return 0;
}
