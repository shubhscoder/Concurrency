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
using Tp = std::chrono::time_point<std::chrono::system_clock>;

class ThreadJoiner {
    vector<thread>& joiner_;
    public:

    explicit ThreadJoiner(vector<thread>& threads) : joiner_(threads) {}
    ~ThreadJoiner() {
        for (auto& t: joiner_) {
            if (t.joinable()) {
                t.join();
            }
        }
    }
};

enum TaskType {
    kOneTime,
    kFixedRate,
    kFixedDelay
};

struct Task {
    function<void()> task;
    TaskType type;
    Tp start_time;
    long delay; // in nanoseconds.

    Task() {}
    Task(function<void()> tsk, TaskType typ, Tp start, long del) : 
    task(tsk),
    type(typ),
    start_time(start),
    delay(del) {}

    bool operator<(const Task& other) const {
        return start_time < other.start_time;
    }
};

class TaskSchedulerSimple {
    private:
    
    mutex mt_;
    condition_variable cnd_;
    atomic_bool done_;
    using Qele = std::pair<Tp, Task>;
    // set<Qele> tasks_;
    set<Task> tasks_;
    vector<thread> threads_;
    shared_ptr<ThreadJoiner> joiner_;

    Tp cur_time() {
        return std::chrono::system_clock::now();
    }

    Tp new_start_time(Tp start_time, long delay) {
        std::chrono::seconds delay_duration(delay);
        Tp new_point = start_time + delay_duration;
        return new_point;
    } 

    bool check() {
        return false;
    }

    void timer_thread() {
        while (true) {
            {
                unique_lock<mutex> lk(mt_);
                if (!tasks_.empty() && tasks_.begin()->start_time <= cur_time()) {
                    lk.unlock();
                    cnd_.notify_one();
                }
            }
        }
    }

    private:

    void PollTask() {
        Task cur_task;
        while (true) {
            {
                unique_lock<mutex> lk(mt_);
                // cnd_.wait(lk, [&]() { return (!tasks_.empty() && tasks_.begin()->start_time <= cur_time()) || done_;});
                cnd_.wait(lk, [&]() { return !tasks_.empty() || done_;});
                if (done_) {
                    break;
                }

                // if (tasks_.empty() || tasks_.begin()->start_time > cur_time()) {
                //     this_thread::yield();
                //     continue;
                // }
                
                while (!tasks_.empty()) {
                    if (tasks_.begin()->start_time <= cur_time()) {
                        break;
                    }
                    cnd_.wait_until(lk, tasks_.begin()->start_time, [&](){ return !tasks_.empty() && .begin()->start_time <= cur_time(); });
                }

                auto res = *tasks_.begin();
                cur_task = res;
                tasks_.erase(tasks_.begin());
            }

            switch (cur_task.type)
            {
                case kOneTime:
                    cur_task.task();
                    break;
                case kFixedRate:
                    cur_task.start_time = new_start_time(cur_task.start_time, cur_task.delay);
                    {
                        lock_guard<mutex> lk(mt_);
                        tasks_.insert(cur_task);
                    }
                    cnd_.notify_all();
                    cur_task.task();
                    break;
                case kFixedDelay:
                    cur_task.task();
                    cur_task.start_time = new_start_time(cur_time(), cur_task.delay);
                    {
                        lock_guard<mutex> lk(mt_);
                        tasks_.insert(cur_task);
                    }
                    cnd_.notify_all();
                    break;
                default:
                    break;
            }
        }
    }

    public:
    TaskSchedulerSimple(int num = std::thread::hardware_concurrency()) :
    done_(false),
    joiner_(make_shared<ThreadJoiner>(threads_)) {
        for (int i = 0; i < num - 1; i++) {
            threads_.push_back(thread(&TaskSchedulerSimple::PollTask, this));
        }
        threads_.push_back(thread(&TaskSchedulerSimple::timer_thread, this));
    }

    void schedule(function<void()> task, long delay) {
        auto cur = cur_time();
        // convert(cur);
        auto new_start = new_start_time(cur, delay);
        // convert(new_start);
        Task tsk;
        tsk.task = task;
        tsk.type = kOneTime;
        tsk.start_time = new_start_time(cur_time(), delay);
        tsk.delay = delay;

        {
            lock_guard<mutex> lk(mt_);
            tasks_.insert(tsk);
        }
        cnd_.notify_all();
    }

    void scheduleAtFixedRate(function<void()> task, long delay, long period) {
        Task tsk;
        tsk.task = task;
        tsk.type = kFixedRate;
        auto cur = cur_time();
        auto new_start = new_start_time(cur, delay);
        tsk.start_time = new_start;
        tsk.delay = period;

        {
            lock_guard<mutex> lk(mt_);
            tasks_.insert(tsk);
        }
        cnd_.notify_all();
    }

    void scheduleWithFixedDelay(function<void()> task, long delay, long period) {
        Task tsk;
        tsk.task = task;
        tsk.type = kFixedDelay;
        auto cur = cur_time();
        auto new_start = new_start_time(cur, delay);
        tsk.start_time = new_start;
        tsk.delay = period;

        {
            lock_guard<mutex> lk(mt_);
            tasks_.insert(tsk);
        }
        cnd_.notify_all();    
    }

    void convert(std::chrono::system_clock::time_point now) {
        // Convert to a time_t (seconds since epoch)
        std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);

        // Convert to a struct tm (broken down time)
        std::tm* local_time = std::localtime(&time_t_now);

        // Format the time as a readable clock
        char buffer[100];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local_time);

        // Print the result
        std::cout << "Current time: " << buffer << std::endl;
    }

    // void test() {
    //     auto tt = std::chrono::system_clock::now();
    //     auto tt2 = std::chrono::system_clock::now() + std::chrono::seconds(120);
    //     convert(tt);
    //     convert(tt2);
    // }
};

long nano_sec = 1;

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
    TaskSchedulerSimple sch;
    // sch.test();

    long delay = 3;
    sch.schedule(OneTimeTask, delay);

    // delay = 1;
    // sch.scheduleAtFixedRate(FixedRateTask, delay, 3);

    // delay = 2*nano_sec;
    // sch.scheduleWithFixedDelay(FixedDelayTask, delay, 5 * nano_sec);
}

int main() {
    Test();
}