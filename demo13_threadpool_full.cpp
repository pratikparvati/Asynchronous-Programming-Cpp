#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>

class ThreadPool {
public:
    ThreadPool(size_t threads);
    ~ThreadPool();

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

private:
    std::vector<std::thread> workers;  // Worker threads
    std::queue<std::function<void()>> tasks;  // Task queue

    std::mutex queue_mutex;  // Mutex for task queue
    std::condition_variable condition;  // Condition variable for task queue
    bool stop;  // Stopping flag

    // Mutex for synchronizing console output
    std::mutex print_mutex;
};

// Constructor: Initialize worker threads
ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this, i] {
            for (;;) {
                std::function<void()> task;

                {  // Acquire lock
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });

                    if (this->stop && this->tasks.empty()) return;

                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }  // Release lock

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
    {  // Acquire lock
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }  // Release lock

    condition.notify_all();  // Notify all threads
    for (std::thread &worker : workers) worker.join();
}

// Add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();

    {  // Acquire lock
        std::unique_lock<std::mutex> lock(queue_mutex);

        if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task]() { (*task)(); });
    }  // Release lock

    condition.notify_one();
    return res;
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
    // Create a thread pool with 4 threads
    ThreadPool pool(4);

    // Mutex for synchronizing console output in tasks
    std::mutex print_mutex;

    // Enqueue 10 tasks
    std::vector<std::future<void>> results;
    for (int i = 0; i < 10; ++i) {
        results.emplace_back(pool.enqueue(example_function, i, std::ref(print_mutex)));
    }

    // Wait for all tasks to complete
    for (auto &&result : results) result.get();

    return 0;
}
