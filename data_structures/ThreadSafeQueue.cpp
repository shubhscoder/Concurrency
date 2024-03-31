#include <iostream>
#include <stack>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <exception>

using namespace std;

template <typename T>
class ThreadSafeQueue {
    queue<shared_ptr<T> > q_;
    mutex q_mt_;
    condition_variable q_cnd_;

    public:

    void Push(T val) {
        shared_ptr<T> val_ptr = make_shared<T> (val);
        lock_guard<mutex> lk(q_mt_);
        q_.push(val_ptr);
        q_cnd_.notify_one();
    }

    shared_ptr<T> WaitAndPop() {
        unique_lock<mutex> lk(q_mt_);
        q_cnd_.wait(lk, [this] () { return !q_.empty();});
        shared_ptr<T> val_ptr = q_.front();
        q_.pop();
        return val_ptr;
    }

    shared_ptr<T> TryPop() {
        lock_guard<mutex> lk(q_mt_);
        if (q_.empty()) {
            return shared_ptr<T>();
        }
        shared_ptr<T> val_ptr = q_.front();
        q_.pop();
        return val_ptr;
    }

    shared_ptr<T> TimedWaitAndPop(int duration_ms) {
        auto timeout = chrono::system_clock::now() + chrono::milliseconds(duration_ms);
        unique_lock<mutex> lk(q_mt_);
        shared_ptr<T> val_ptr;
        if (q_cnd_.wait_until(lk, timeout, [this] () { return !q_.empty();})) {
            val_ptr = q_.front();
            q_.pop();
        }
        return val_ptr;
    }
};