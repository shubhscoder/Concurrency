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
#include <set>

using namespace std;

using Tp = std::chrono::system_clock::time_point;

enum Tasktype {
    kOnetime,
    kFixedRate,
    kFixedDelay
};

struct Task {
    function<void()> job;
    Tasktype type;
    Tp start_time;
    long period;

    Task() {}
    Task(function<void()> job_, Tasktype type_, Tp start_time_) :
    job(job_),
    type(type_),
    start_time(start_time_) {}

    Task(function<void()> job_, Tasktype type_, Tp start_time_, long period_) :
    job(job_),
    type(type_),
    start_time(start_time_),
    period(period_) {}

    bool operator<(const Task& other) const {
        return start_time < other.start_time;
    }
};

class ThreadJoiner {
    vector<thread>& threads_;
    public:
    explicit ThreadJoiner(vector<thread>& threads) : threads_(threads) {}
    ~ThreadJoiner() {
        for (auto& t: threads_) {
            if (t.joinable()) {
                t.join();
            }
        } 
    }
};

class Scheduler {
    private:
    mutex mt_;
    condition_variable cnd_;
    set<Task> tasks_;
    atomic_bool done_;
    vector<thread> threads_;
    unique_ptr<ThreadJoiner> joiner_;

    Tp cur_time() {
        return std::chrono::system_clock::now();
    }

    // Call holding lock
    bool check_task_available() {
        return !tasks_.empty() && tasks_.begin()->start_time <= cur_time();
    }

    Tp new_start_time(Tp cur, long delay) {
        return cur + std::chrono::milliseconds(delay);
    }

    void insertCurrentTask(Task& tsk) {
        switch(tsk.type) {
            case kFixedDelay:
            tsk.start_time = new_start_time(cur_time(), tsk.period);
            break;

            case kFixedRate:
            tsk.start_time = new_start_time(tsk.start_time, tsk.period);
            break;

            default:
            return;
        }

        {
            std::lock_guard<mutex> lk(mt_);
            tasks_.insert(tsk);
        }
    }

   void PollTask() {
        Task cur_task;
        while (true) {
            {
                unique_lock<mutex> lk(mt_);
                cnd_.wait(lk, [&]() {
                    return !tasks_.empty() || done_.load();
                });

                if (done_) {
                    cout << "Done, breaking\n";
                    break;
                }

                while (!tasks_.empty()) {
                    if (check_task_available()) {
                        break;
                    }

                    cnd_.wait_until(lk, tasks_.begin()->start_time, [&]() {
                        return !tasks_.empty() && check_task_available();
                    });
                }

                cur_task = *tasks_.begin();
                tasks_.erase(tasks_.begin());
            }

            function<void()> callback;
            switch (cur_task.type) {
                case kOnetime:
                cur_task.job();
                break;

                case kFixedDelay:
                callback =  [&, cur_tsk = cur_task] () mutable {
                    cur_tsk.job();
                    // As soon as task is complete, add task to queue again
                    insertCurrentTask(cur_tsk);
                };
                std::async(callback);
                break;
    
                case kFixedRate:
                insertCurrentTask(cur_task);
                std::async(cur_task.job);
                break;
            }
        }
    }

    public:
    Scheduler() : joiner_(make_unique<ThreadJoiner> (threads_)) {
        threads_.push_back(thread(&Scheduler::PollTask, this));
    }

    void shutdown() {
        done_.store(true);
        cnd_.notify_all();
    }

    void schedule(function<void()> func, long delay_ms) {
        Task cur_task(func, kOnetime, new_start_time(cur_time(), delay_ms));
        {
            lock_guard<mutex> lk(mt_);
            tasks_.insert(cur_task);
        }
        cnd_.notify_one();
    }

    void scheduleAtFixedRate(function<void()> func, long delay_ms, long period_ms) {
        Task cur_task(func, kFixedRate, new_start_time(cur_time(), delay_ms), period_ms);
        {
            lock_guard<mutex> lk(mt_);
            tasks_.insert(cur_task);
        }
        cnd_.notify_one();
    }

    void scheduleFixedDelay(function<void()> func, long delay_ms, long period_ms) {
        Task cur_task(func, kFixedDelay, new_start_time(cur_time(), delay_ms), period_ms);
        {
            lock_guard<mutex> lk(mt_);
            tasks_.insert(cur_task);
        }
        cnd_.notify_one();
    }
};

void OneTimeTask() {
    cout << "OneTimeTask\n";
}

void FixedRateTask() {
    cout << "FixedRateTask\n";
}

void FixedDelayTask() {
    cout << "FixedDelayTask\n";
}

void Test() {
    Scheduler sch;
    // sch.test();

    long delay = 3000;
    sch.schedule(OneTimeTask, delay);

    delay = 1000;
    sch.scheduleAtFixedRate(FixedRateTask, delay, delay * 2);

    delay = 2000;
    sch.scheduleFixedDelay(FixedDelayTask, delay, 3000);
}

int main() {
    Test();
}