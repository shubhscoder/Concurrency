#include <iostream>
#include <thread>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <shared_mutex>
#include <future>
#include <algorithm>
#include <functional>
#include <numeric>

using namespace std;

class ThreadJoiner {
    vector<thread>& threads_;
    public:

    explicit ThreadJoiner(vector<thread>& threads) : threads_(threads) {}
    ~ThreadJoiner() {
        for (auto& th: threads_) {
            if (th.joinable()) {
                th.join();
            }
        }
    }
};

class ThreadPool {
    private:
    mutex mt_;
    condition_variable cnd_;
    atomic<bool> done_;
    queue<function<void()> > tasks_;
    vector<thread> pool_;
    shared_ptr<ThreadJoiner> joiner_;

    void PollTask() {
        while (true) {
            function<void()> task;
            {
                unique_lock<mutex> lk(mt_);
                cnd_.wait(lk, [&]() {
                    return !tasks_.empty() || done_;
                });

                if (done_) {
                    break;
                }
                task = tasks_.front();
                tasks_.pop();
            }
            task();
        }
    }

    template<typename ReturnType, typename Callback>
    static void execute(std::promise<ReturnType>& p, Callback& c) {
        p.set_value(c());
    }

    template<typename Callback>
    static void execute(std::promise<void>& p, Callback& c) {
        c();
        p.set_value();
    }

    public:
    explicit ThreadPool(int num_threads = std::thread::hardware_concurrency()) : 
    done_(false),
    joiner_(make_shared<ThreadJoiner>(pool_)) {
        for (int it = 0; it < num_threads; it++) {
            pool_.push_back(thread(&ThreadPool::PollTask, this));
        }
    }

    template<typename Callback>
    auto AddTask(Callback task) -> std::future<decltype(task())> {
        typedef decltype(task()) ReturnType;
        shared_ptr<promise<ReturnType> > prom = make_shared<promise<ReturnType> >();
        auto result = prom->get_future();
        function<void()> wrapper = [p = std::move(prom), tsk = std::move(task)] () mutable {
            execute(*prom, tsk);
        };

        {
            lock_guard<mutex> lk(mt_);
            tasks_.push(std::move(wrapper));
        }
        cnd_.notify_one();
    }

    void StopPool() {
        done_.store(true);
        cnd_.notify_all();
    }
};

int main() {

}