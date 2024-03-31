#include <iostream>
#include <thread>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <vector>

using namespace std;

class CustomSemaphore {
    int cnt_;
    std::mutex cnt_mut_;
    std::condition_variable cnt_cnd_;

    public:
    CustomSemaphore(int cnt) : cnt_(cnt) {}

    void signal() {
       std::lock_guard<std::mutex> lk(cnt_mut_);
       cnt_++;
       cnt_cnd_.notify_one();
    }

    void wait() {
        std::unique_lock<std::mutex> lk(cnt_mut_);
        cnt_cnd_.wait(lk, [this](){return cnt_ > 0;}); // If the condition holds true, then waiting is done.
        cnt_--;
    }
};

class Barrier {
    int num_threads_;
    int cnt_;
    shared_ptr<CustomSemaphore> mutex_;
    shared_ptr<CustomSemaphore> barrier_;

    public:
    Barrier(int num_threads) : num_threads_(num_threads),
    mutex_(make_shared<CustomSemaphore>(1)),
    barrier_(make_shared<CustomSemaphore>(0)),
    cnt_(0) {}

    void barrier_wait() {
        mutex_->wait();
        cnt_++;
        bool flag = (cnt_ == num_threads_);
        mutex_->signal();
        if (flag) {
            // all threads have reached the barrier.
            barrier_->signal();
        } else {
            barrier_->wait();
            barrier_->signal();
        }
    }

    void Function() {
        cout << "Before barrier" << endl << std::flush;
        barrier_wait();
        cout << "After barrier" << endl << std::flush;
    }
};

class ReUsableBarrier {
    int num_threads_;
    int cnt_;
    shared_ptr<CustomSemaphore> mutex_;
    shared_ptr<CustomSemaphore> barrier_;
    shared_ptr<CustomSemaphore> barrier2_;

    public:
    ReUsableBarrier(int num_threads) : num_threads_(num_threads),
    mutex_(make_shared<CustomSemaphore>(1)),
    barrier_(make_shared<CustomSemaphore>(0)),
    barrier2_(make_shared<CustomSemaphore>(1)),
    cnt_(0) {}

    void barrier_phase_1() {
        mutex_->wait();
        cnt_++;
        bool flag = (cnt_ == num_threads_);
        mutex_->signal();

        if (flag) {
            barrier2_->wait();
            for (int i = 0; i < num_threads_; i++) {
                barrier_->signal();
            }
        } else {
            barrier_->wait();
        }
    }

    void barrier_phase_2() {
        mutex_->wait();
        cnt_--;
        bool flag = (cnt_ == 0);
        mutex_->signal();

        if (flag) {
            // last thread. lock the barrier
            barrier_->wait();
            for (int i = 0; i < num_threads_; i++) {
                barrier2_->signal();
            }
        } else {
            barrier2_->wait();
        }
    }

    void barrier_wait() {
        barrier_phase_1();
        barrier_phase_2();
    }

    void Function() {
        for (int i = 0; i < 5; i++) {
            printf("Before barrier %d\n", i);
            barrier_wait();
            printf("After barrier %d\n", i);
        }
    }
};

class CustomResusableBarrier {
    int num_threads_;
    int cnt_;
    shared_ptr<CustomSemaphore> mutex_;
    shared_ptr<CustomSemaphore> barrier_;
    shared_ptr<CustomSemaphore> barrier2_;
    int sense_;

    public:
    CustomResusableBarrier(int num_threads) : num_threads_(num_threads),
    cnt_(0),
    mutex_(make_shared<CustomSemaphore>(1)),
    barrier_(make_shared<CustomSemaphore>(0)),
    barrier2_(make_shared<CustomSemaphore>(0)),
    sense_(1) {}

    void barrier_wait() {
        mutex_->wait();
        int current_sense = sense_;
        cnt_ += sense_;
        if (sense_ == 1) {
            if (cnt_ == num_threads_) {
                // printf("Reversing sense for the first time\n");
                sense_ *= -1;
                // nth thread has arrived.
                for (int i = 0; i < num_threads_; i++) {
                    barrier_->signal();
                }
            }
        } else {
            if (cnt_ == 0) {
                // printf("Reversing sense for the second time\n");
                sense_ *= -1;
                // last thread arrived again.
                for (int i = 0; i < num_threads_; i++) {
                    barrier2_->signal();
                }
            }
        }
        mutex_->signal();

        if (current_sense == 1) {
            barrier_->wait();
        } else {
            barrier2_->wait();
        }
    }

    void Function() {
        for (int i = 0; i < 5; i++) {
            printf("Before barrier %d\n", i);
            barrier_wait();
            printf("After barrier %d\n", i);
        }
    }
};

void BarrierTest() {
    vector<unique_ptr<thread>> threads;
    int num_threads = 2;
    Barrier test(num_threads);
    for (int i = 0; i < num_threads; i++) {
        threads.push_back(make_unique<thread>(&Barrier::Function, &test));
    }

    for (auto& t: threads) {
        t->join();
    }
}

void ReUsableBarrierTest() {
    vector<unique_ptr<thread>> threads;
    int num_threads = 5;
    ReUsableBarrier test(num_threads);
    for (int i = 0; i < num_threads; i++) {
        threads.push_back(make_unique<thread>(&ReUsableBarrier::Function, &test));
    }

    for (auto& t: threads) {
        t->join();
    }
}

void CustomResusableBarrierTest() {
    vector<unique_ptr<thread>> threads;
    int num_threads = 5;
    CustomResusableBarrier test(num_threads);
    for (int i = 0; i < num_threads; i++) {
        threads.push_back(make_unique<thread>(&CustomResusableBarrier::Function, &test));
    }

    for (auto& t: threads) {
        t->join();
    }
}

int main() {
    // CustomResusableBarrierTest();
    ReUsableBarrierTest();
}