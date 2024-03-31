#include <iostream>
#include <thread>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <vector>

using namespace std;

class CustomSemaphore {
    int cnt_;
    mutex cnt_mt_;
    condition_variable cnt_cond_;

    public:
    CustomSemaphore() : cnt_(0) {}

    void wait() {
        
    }
};