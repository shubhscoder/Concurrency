/*
Similarly, in order for a thread to access a shared variable, it has to “get”
the mutex; when it is done, it “releases” the mutex. Only one thread can hold
the mutex at a time.
Puzzle: Add semaphores to the following example to enforce mutual exclusion to the shared variable count.

Thread A
count = count + 1

Thread B
count = count + 1
*/

#include <iostream>
#include <thread>
#include <memory>
#include <condition_variable>
#include <mutex>

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

class MutexTest {
    int cnt_;
    shared_ptr<CustomSemaphore> mt_;

    public:
    MutexTest() : cnt_(0), mt_(make_shared<CustomSemaphore>(1)) {}

    void IncrementA() {
        mt_->wait();
        cnt_ = cnt_ + 1;
        mt_->signal();
    }

    void IncrementB() {
        mt_->wait();
        cnt_ = cnt_ + 1;
        mt_->signal();
    }

    void Print() {
        cout << cnt_ << endl;
    }
};

int main() {
    MutexTest mt;
    thread A(&MutexTest::IncrementA, &mt);
    thread B(&MutexTest::IncrementB, &mt);

    A.join();
    B.join();

    mt.Print();
}

/*
Outputs:
2
*/

/*
How to generalize the previous solution?

Change initializaiton of the mutex to n
*/