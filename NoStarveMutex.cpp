#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>

using namespace std;

class CustomSemaphore {
    int cnt_;
    mutex cnt_mt_;
    condition_variable cnt_cond_;

    public:
    CustomSemaphore(int cnt) : cnt_(cnt) {}

    void wait() {
        std::unique_lock<mutex> lk(cnt_mt_);
        cnt_cond_.wait(lk, [&](){ return cnt_ > 0;});
        cnt_--;
    }

    void signal() {
        std::lock_guard<mutex> lk(cnt_mt_);
        cnt_++;
        cnt_cond_.notify_one();
    }
};

class NoStarveMutex {
    // Number of people waiting at door 1.
    int cnt1_;

    /// Number of people waiting at door 2.
    int cnt2_;

    // Mutex for exclusive access to the doors.
    shared_ptr<CustomSemaphore> cnt_mt_;

    // Door 1
    shared_ptr<CustomSemaphore> door1_;

    // Door 2
    shared_ptr<CustomSemaphore> door2_;

    // door_.wait() indicates waiting for door to open
    // door_.signal is door opening, letting one guy go inside
    // and closing the door again. That guy will be one of the 
    // guys waiting.

    public:
    NoStarveMutex() : cnt1_(0),
                      cnt2_(0),
                      cnt_mt_(make_shared<CustomSemaphore>(1)),
                      door1_(make_shared<CustomSemaphore>(1)),
                      door2_(make_shared<CustomSemaphore>(0)) {}
    // door1_ initially open, letting threads wait for door 2
    // door2_ intially closed.

    void morris() {
        // thread has arrived. Always at door1_
        // therefore, increment that count.
        cnt_mt_->wait();
        cnt1_++;
        cnt_mt_->signal();

        // check if door 1 is open
        door1_->wait();
        // door open, go from waiting outside door1 to door 2
        cnt2_++;
        // Need mutex here because someone above could be modifying the cnt1
        cnt_mt_->wait();
        cnt1_--;
        bool flag = (cnt1_ == 0);
        cnt_mt_->signal();

        if (flag) {
            // last thread to come from door 1 to door 2
            door2_->signal();
        } else {
            // some threads still waiting at door 1.
            // Let them come inside.
            door1_->signal();
        }

        // Now let them go through door2, which is critical.
        // Check if door2 is open
        door2_->wait();
        cnt2_--;
        flag = (cnt2_ == 0);
        if (flag) {
            // last one to go through  the critical section.
            door1_->signal();
        } else {
            // some still waiting at door 2
            door2_->signal();
        }

        // Critical section.
        // Rest of the code.
    }
};