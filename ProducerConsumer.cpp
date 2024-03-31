#include <iostream>
#include <thread>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>

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

struct Task {
    string command_;
};

class ProducerConsumer {
    queue<shared_ptr<Task> > tasks_;
    shared_ptr<CustomSemaphore> mt_;
    shared_ptr<CustomSemaphore> task_sig_;
    shared_ptr<CustomSemaphore> buffer_size_;

    public:

    ProducerConsumer(int buffer_sz) : mt_(make_shared<CustomSemaphore>(1)),
    task_sig_(make_shared<CustomSemaphore>(0)),
    buffer_size_(make_shared<CustomSemaphore>(buffer_sz)) {}

    void ProcessTask(shared_ptr<Task> tsk) {
        printf("Command : %s\n", tsk->command_.c_str());
    }

    void Producer(shared_ptr<Task> tsk) {
        buffer_size_->wait();
        mt_->wait();
        tasks_.push(tsk);
        mt_->signal();
        task_sig_->signal();
    } 

    void Consumer() {
        task_sig_->wait();
        mt_->wait();
        auto tsk = tasks_.front();
        tasks_.pop();
        mt_->signal();
        buffer_size_->signal();
        ProcessTask(tsk);
    }
};

template <typename T> 
class ConcurrentQueue {
    queue<shared_ptr<T> > q_;
    condition_variable insert_;
    mutex q_mt_;
    chrono::duration<float> wait_time_;
    
    public:

    ConcurrentQueue(int duration_ms = 10) : wait_time_(chrono::milliseconds(duration_ms)) {}
 
    void push(shared_ptr<T> task) {
        std::lock_guard<mutex> lk(q_mt_);
        q_.push(task);
        insert_.notify_one();
    }

    shared_ptr<T> timed_wait_and_pop() {
        std::unique_lock<mutex> lk(q_mt_);
        // 10 s, 50 s
        // 60 s -> jaayega hi jaayega.
        if (!insert_.wait_for(lk, wait_time_, [this]() {return !q_.empty();})) {
            return nullptr;
        }

        shared_ptr<T> item = q_.front();
        q_.pop();
        return item;
    }

    shared_ptr<T> wait_and_pop() {
        std::unique_lock<mutex> lk(q_mt_);
        insert_.wait(lk, [this](){ return !q_.empty();});

        shared_ptr<T> item = q_.front();
        q_.pop();
        return item;
    }

    shared_ptr<T> pop() {
        std::lock_guard<mutex> lk(q_mt_);
        if (q_.empty()) {
            return nullptr;
        }

        shared_ptr<T> item = q_.front();
        q_.pop();
        return item;
    }

    bool empty() {
        std::lock_guard<mutex> lk(q_mt_);
        return q_.empty();
    }
};

class CustomProducerConsumer {
    ConcurrentQueue<Task> q_;
    volatile atomic_bool done_;

    public:

    CustomProducerConsumer() : done_(false) {}

    void Producer(Task tsk) {
        int rnd = 10;
        for (int i = 0; i < 10; i++) {
            shared_ptr<Task> tsk_ptr = make_shared<Task> (tsk);
            q_.push(tsk_ptr);
        }
        done_ = true;
    }

    void ProcessTask(shared_ptr<Task> tsk) {
        printf("Command : %s\n", tsk->command_.c_str());
    }

    void Consumer() {
        shared_ptr<Task> tsk = nullptr;
        while (!done_ || !q_.empty()) {
            tsk = q_.timed_wait_and_pop();
            if (tsk == nullptr) {
                continue;
            }
            shared_ptr<Task> cur_task = tsk;
            tsk = nullptr;
            ProcessTask(cur_task);
        }
    }
};

void ProducerConsumerTest() {
    int prds = 1;
    int cons = 4;
    vector<unique_ptr<thread> > threads;
    CustomProducerConsumer test;

    for (int i = 0; i < prds; i++) {
        Task tsk;
        tsk.command_ = to_string(i);
        threads.push_back(make_unique<thread>(&CustomProducerConsumer::Producer, &test, tsk));
    }

    for (int i = 0; i < cons; i++) {
        Task tsk;
        threads.push_back(make_unique<thread>(&CustomProducerConsumer::Consumer, &test));
    }

    for (auto& t: threads) {
        t->join();
    }

    printf("Test completed Succcessfully\n");
}

int main() {
    ProducerConsumerTest();
}