#include <iostream>
#include <thread>

// Function to be executed by the thread
void printMessage(const std::string& message) {
    std::cout << message << std::endl;
}

int main() {
    // 1. Creating a thread using a function pointer
    std::thread t1(printMessage, "Hello from thread 1!");

    // 2. Creating a thread using a lambda expression
    std::thread t2([]() {
        std::cout << "Hello from thread 2!" << std::endl;
    });

    // 3. Creating a thread using a functor (function object)
    struct Functor {
        void operator()() const {
            std::cout << "Hello from thread 3!" << std::endl;
        }
    };
    Functor functor;
    std::thread t3(functor);

    // 4. Creating a thread using a member function
    class MyClass {
    public:
        void memberFunction(const std::string& message) {
            std::cout << message << std::endl;
        }
    };
    MyClass myObject;
    std::thread t4(&MyClass::memberFunction, &myObject, "Hello from thread 4!");

    // Joining all threads to ensure they complete before the main thread exits
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    std::cout << "All threads have finished." << std::endl;
    return 0;
}
