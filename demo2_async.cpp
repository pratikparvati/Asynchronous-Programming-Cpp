#include <iostream>
#include <future>

// Function to be executed asynchronously
int calculateSquare(int x) {
    return x * x;
}

int main() {
    // 1. Using std::async with a function pointer
    int result1 = std::async(std::launch::async, calculateSquare, 5).get();
    std::cout << "Result from async with function pointer: " << result1 << std::endl;

    // 2. Using std::async with a lambda expression
    int result2 = std::async(std::launch::async, []() -> int {
        return 42;
    }).get();
    std::cout << "Result from async with lambda expression: " << result2 << std::endl;

    // 3. Using std::async with a functor (function object)
    struct Functor {
        int operator()(int x) const {
            return x * x * x;
        }
    };
    Functor functor;
    int result3 = std::async(std::launch::async, functor, 3).get();
    std::cout << "Result from async with functor: " << result3 << std::endl;

    // 4. Using std::async with a member function
    class MyClass {
    public:
        int memberFunction(int x) const {
            return x + 10;
        }
    };
    MyClass myObject;
    int result4 = std::async(std::launch::async, &MyClass::memberFunction, &myObject, 7).get();
    std::cout << "Result from async with member function: " << result4 << std::endl;

    std::cout << "All asynchronous tasks have completed." << std::endl;
    return 0;
}
