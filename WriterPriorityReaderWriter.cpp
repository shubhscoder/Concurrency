#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <memory>

class CustomSemaphore {
    int cnt_;
    std::mutex cnt_mt_;
    std::condition_variable cnt_cond_;

    public:
    CustomSemaphore(int cnt) : cnt_(cnt) {}

    void signal() {
        std::lock_guard<std::mutex> lk(cnt_mt_);
        cnt_++;
        cnt_cond_.notify_one();
    }

    void wait() {
        std::unique_lock<std::mutex> lk(cnt_mt_);
        cnt_cond_.wait(lk, [this](){ return cnt_ > 0; });
        cnt_--;
    }
};

class LightSwitch {
    int cnt_;
    std::shared_ptr<CustomSemaphore> cnt_mt_;

    public:
    LightSwitch() : cnt_(0) {}

    void lock(std::shared_ptr<CustomSemaphore> sema) {
        cnt_mt_->wait();
        cnt_++;
        if (cnt_ == 1) {
            // first one, therefore lock
            sema->wait();
        }
        cnt_mt_->signal();
    }

    void unlock(std::shared_ptr<CustomSemaphore> sema) {
        cnt_mt_->wait();
        cnt_--;
        if (cnt_ == 0) {
            // last one, therefore unlock
            sema->signal();
        }
        cnt_mt_->signal();
    }
};

class ReaderWriter {
    std::shared_ptr<CustomSemaphore> writer_mt_;
    std::shared_ptr<CustomSemaphore> reader_mt_;
    LightSwitch w_switch_;
    LightSwitch r_switch_;

    public:

    void Writer() {
        w_switch_.lock(reader_mt_);
        writer_mt_->wait();
        // CS
        writer_mt_->signal();
        w_switch_.unlock(reader_mt_);
    }

    void Reader() {
        reader_mt_->wait();
        r_switch_.lock(writer_mt_);
        reader_mt_->signal();
        // CS
        r_switch_.unlock(writer_mt_);
    }
};