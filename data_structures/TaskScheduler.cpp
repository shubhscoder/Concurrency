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

/*
Implement following method of ScheduledExecutorService interface in Java

schedule(Runnable command, long delay, TimeUnit unit)
Creates and executes a one-shot action that becomes enabled after the given delay.

scheduleAtFixedRate(Runnable command, long initialDelay, long period, TimeUnit unit)
Creates and executes a periodic action that becomes enabled first after the given initial delay, 
and subsequently with the given period; that is executions will commence after initialDelay then 
initialDelay+period, then initialDelay + 2 * period, and so on.

scheduleWithFixedDelay(Runnable command, long initialDelay, long delay, TimeUnit unit)
Creates and executes a periodic action that becomes enabled first after the given initial delay, and 
subsequently with the given delay between the termination of one execution and the commencement of 
the next.
*/


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
    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> start_time;
    long delay; // in nanoseconds.

    bool operator<(const Task& other) const {
        return start_time < other.start_time;
    }
};

class Scheduler {
    private:

    int num_;
    mutex mt_;
    condition_variable cnd_;
    atomic_bool done_;
    using Tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;
    using Qele = std::pair<Tp, Task>;
    set<Qele> pq_;
    // priority_queue<Qele, vector<Qele>, greater<Qele> > pq_;
    vector<thread> threads_;
    shared_ptr<ThreadJoiner> joiner_;

    private:

    std::chrono::time_point<std::chrono::system_clock> cur_time() {
        return std::chrono::system_clock::now();
    }

    // void printTime(std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> cur) {
    //     auto tm = std::chrono::system_clock::to_time_t(cur);
    //     cout << "Time: " << std::ctime(&tm) << std::endl;
    // }

    void PollTask() {
        Qele cur_task;

        while (!done_) {
            {
                unique_lock<mutex> lk(mt_);
                cnd_.wait(lk, [&](){ 
                    // printTime(pq_.begin()->first);
                    // printTime(cur_time());
                    cout << (pq_.begin()->first <= cur_time()) << endl; 
                    return (!pq_.empty() || done_); 
                });

                if (pq_.begin()->first <= cur_time()) {
                    cout << "Found scheduler\n";
                } else {
                    cout << "Not found\n";
                    continue;
                }

                cout << "done waiting"<< endl;
                if (done_) {
                    break;
                }
                
                cout << "Task available for schedule" << endl;
                
                // if (pq_.begin()->first <= std::chrono::system_clock::now()) {
                    cout << "Test2" << endl; 
                    // The task is available for pickup
                    cur_task = *pq_.begin();
                    cout << cur_task.second.delay << endl;
                    pq_.erase(pq_.begin());
                // }

                if (cur_task.second.type == kFixedRate) {
                    auto new_start = cur_task.second.start_time +  std::chrono::nanoseconds(cur_task.second.delay);
                    Task new_task {
                        cur_task.second.task,
                        kFixedRate,
                        new_start,
                        cur_task.second.delay
                    };
                    pq_.insert({new_start, new_task});
                }

                cur_task.second.task();
                if (cur_task.second.type == kFixedDelay) {
                    auto new_start = std::chrono::system_clock::now() + std::chrono::nanoseconds(cur_task.second.delay);
                    Task new_task {
                        cur_task.second.task,
                        kFixedDelay,
                        new_start,
                        cur_task.second.delay
                    };
                    pq_.insert({new_start, new_task});
                }
            }
        }
    }

    template<typename Callback, typename ReturnType>
    void execute_func(shared_ptr<promise<ReturnType>> prom, Callback& func) {
        prom->set_value(func());
    }

    template<typename Callback>
    void execute_func(shared_ptr<promise<void>> prom, Callback& func) {
        func();
        prom->set_value();
    }

    template<typename Callback>
    auto getRunnable(Callback&& func) {
        using ReturnType = decltype(func());
        shared_ptr<promise<ReturnType> > prom = make_shared<promise<ReturnType> >();
        function<void()> task = [&]() {
            execute_func(*prom, func);
        };

        return make_pair(task, prom);
    }


    public:

    Scheduler(int num = std::thread::hardware_concurrency()) : num_(num),
    joiner_(make_shared<ThreadJoiner>(threads_)),
    done_(false) {
        for (int i = 0; i < num_; i++) {
            try {
            threads_.push_back(thread(&Scheduler::PollTask, this));
            } catch (const std::exception& ex) {
                std::cout << "Rejected with exception: " << ex.what() << std::endl;

}        }
    }

    template<typename Callback>
    auto schedule(Callback&& func, long delay) {
        using ReturnType = decltype(func());
        shared_ptr<promise<ReturnType> > promp = make_shared<promise<ReturnType> >();
        auto result = promp->get_future();
        function<void()> runnable = [this, prom = std::move(promp), fun = std::move(func)]() {
            execute_func(prom, fun);
        };

        auto start_time = std::chrono::system_clock::now() + std::chrono::nanoseconds(delay);
        Task task {
            runnable,
            kOneTime,
            start_time,
            delay
        };
        {
            lock_guard<mutex> lk(mt_);
            pq_.insert({start_time, task});
        }
        cout << "done inserting task" << endl;
        cnd_.notify_one();

        return result;
    }

    template<typename Callback>
    auto scheduleAtFixedRate(Callback&& func, long delay, long period) {
        using ReturnType = decltype(func());
        shared_ptr<promise<ReturnType> > promp = make_shared<promise<ReturnType> >();
        auto result = promp->get_future();
        function<void()> runnable = [this, prom = std::move(promp), fun = std::move(func)]() {
            execute_func(prom, fun);
        };

        auto start_time = std::chrono::system_clock::now() + std::chrono::nanoseconds(delay);
        Task task {
            runnable,
            kFixedRate,
            start_time,
            period
        };
        {
            lock_guard<mutex> lk(mt_);
            pq_.insert({start_time, task});
        }
        cnd_.notify_one();

        return result;
    }

    template<typename Callback>
    auto scheduleWithFixedDelay(Callback&& func, long delay, long period) {
        using ReturnType = decltype(func());
        shared_ptr<promise<ReturnType> > promp = make_shared<promise<ReturnType> >();
        auto result = promp->get_future();
        function<void()> runnable = [this, prom = std::move(promp), fun = std::move(func)]() {
            execute_func(prom, fun);
        };

        auto start_time = std::chrono::system_clock::now() + std::chrono::nanoseconds(delay);
        Task task {
            runnable,
            kFixedRate,
            start_time,
            period
        };
        {
            lock_guard<mutex> lk(mt_);
            pq_.insert({start_time, task});
        }
        cnd_.notify_one();

        return result;
    }
};

long nano_sec = 1000000000;

void OneTimeTask() {
    cout << "OneTimeTask" << endl;
}

void FixedRateTask() {
    cout << "FixedRateTask" << endl;
}

void FixedDelayTask() {
    cout << "FixedRateTask" << endl;
}

void Test() {
    Scheduler sch;

    long delay = 3*nano_sec;
    auto res = sch.schedule(OneTimeTask, delay);
    res.get();

    delay = 1*nano_sec;
    auto res1 = sch.scheduleAtFixedRate(FixedRateTask, delay, 1*nano_sec);

    delay = 2*nano_sec;
    auto res2 = sch.scheduleWithFixedDelay(FixedDelayTask, delay, 3 * nano_sec);
}

int main() {
    Test();
}