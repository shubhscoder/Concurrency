#include <iostream>
#include <thread>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <shared_mutex>
#include <future>

using namespace std;

class Factorial {
    public:
    int calculate(int x) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        int result = 1;
        for (int  i = 2; i <= x; i++) {
            result *= i;
        }

        return result;
    }

    int calculate2(future<int>& x) {
        int result = 1;
        int xlim = x.get();
        for (int i = 2; i <= xlim; i++) {
            result *= i;
        }

        return result;
    }
};

void testFactorial() {
    Factorial fact;

    //1. thread t1(&Factorial::calculate, &fact, 5);
    future<int> val = async(&Factorial::calculate, &fact, 4);
    cout << "Doing some intermediate tasks" << endl;
    cout << "Waiting for factorial " << endl;
    cout << "Got factorial " << val.get() << endl;
}

void testFactorialPromise() {
    Factorial fact;

    promise<int> p;
    future<int> fu = p.get_future();
    future<int> result = async(&Factorial::calculate2, &fact, std::ref(fu));
    cout << "Collecting value for the parameters" << endl;
    p.set_value(4);
    cout << "Value is set, got factorial : " << result.get() << endl;
}

int main() {
    // testFactorial();
    testFactorialPromise();
}

