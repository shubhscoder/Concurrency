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
class ThreadPool {
    private:
    int n_;
    queue<function<void()> > tasks_;
    mutex mt_;
    condition_variable cnd_;
    atomic_bool done_;
    vector<thread> threads_;
    shared_ptr<ThreadJoiner> joiner_;

    private:

    void PollTask() {
        while (true) {
            function<void()> task;
            {
                unique_lock<mutex> lk(mt_);
                cnd_.wait(lk, [&]() { return !tasks_.empty() || done_; });
                if (done_) {
                    break;
                }

                task = tasks_.front();
                tasks_.pop();
            }
            task();
        }
    }

    template <typename Callback, typename ReturnType>
    void execute_func(shared_ptr<promise<ReturnType> > prom, Callback& func) {
        try {
            prom->set_value(func());
        } catch (const std::exception& e) {
            prom->set_exception(std::current_exception());
            std::cerr << "Exception caught :" << e.what() << std::endl;
        }
    }

    template <typename Callback>
    void execute_func(shared_ptr<promise<void> > prom, Callback& func) {
        try {
            func();
            prom->set_value();
        } catch (const std::exception& e) {
            prom->set_exception(std::current_exception());
            std::cerr << "Exception caught : " << e.what() << std::endl;
        }
    }

    public:

    ThreadPool(int n = std::thread::hardware_concurrency()) : n_(n),
    done_(false),
    joiner_(make_shared<ThreadJoiner>(threads_)) {
        for (int i = 0; i < n_; i++) {
            threads_.push_back(thread(&ThreadPool::PollTask, this));
        }
    }

    template <typename Callback>
    auto AddTask(Callback&& task) -> future<decltype(task())> {
        typedef decltype(task()) ReturnType;
        shared_ptr<promise<ReturnType> > prom = make_shared<promise<ReturnType> >();
        auto result = prom->get_future();
        function<void()> task_func = [this, p = prom, func = std::move(task)] () mutable {
            execute_func(p, func);
        };

        {
            lock_guard<mutex> lk(mt_);
            tasks_.push(std::move(task_func));
        }
        cnd_.notify_one();

        return result;
    }

    void shutdown() {
        done_.store(true);
        cnd_.notify_all();
    }

};

void BasicTask() {
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

int BasicTaskIntReturn() {
    return 42;
}

int factorial(int a) {
    int result = a;
    for (int cur = a - 1; cur > 1; cur--) {
        result *= cur;
    }

    return result;
}

vector<string> urls(string url);

vector<string> forw(string url) {
    return urls(url);
}

vector<string> urls(string url) {
    string temp = "aaaa";
    vector<string> ans;
    for (int i = 0; i < 10; i++) {
        url += temp;
        ans.push_back(url);
    }

    return ans;
}

int tempo() {
    throw std::runtime_error("Something went wrong with this thread");
    return 5;
}

void done() {
    cout << "DONE\n";
}

void test() {
    ThreadPool pool;
    pool.AddTask(BasicTask);

    auto result = pool.AddTask(BasicTaskIntReturn);
    cout << "Result of return type task " << result.get() << endl;

    auto result2 = pool.AddTask(std::bind(factorial, 5));
    cout << "Result of factorial task " << result2.get() << endl;

    string temp = "b";
    auto result3 = pool.AddTask(std::bind(forw, temp));
    auto resi = result3.get();
    for (auto it: resi) cout << it << " ";cout << endl;

    auto result4 = pool.AddTask(std::bind(tempo));
    try {
        cout << "Bad task :" << result4.get() << endl;
    } catch (const exception& e) {

    }

    auto res5 = pool.AddTask(std::bind(done));
    res5.get();
    pool.shutdown();
}

int main() {

    test();
}
