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

typedef unsigned long ul;

using namespace std;
const int min_per_thread = 25;

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

template <typename Iter, typename T>
T func(Iter blk_start, Iter blk_end) {
    return std::accumulate(blk_start, blk_end, T{});
}

template <typename Iter, typename T>
T Accumulate(Iter start_it, Iter end_it, T init) {
    ul sz = distance(start_it, end_it);
    if (sz <= 25) {
        return std::accumulate(start_it, end_it, init);
    }

    ul max_threads = sz + (min_per_thread - 1) / min_per_thread;
    ul hw_threads = std::thread::hardware_concurrency();
    ul num_threads = min(hw_threads ? hw_threads : 2, max_threads);
    ul blk_sz = sz / num_threads;

    vector<future<T> > futures(num_threads - 1);
    vector<thread> threads(num_threads - 1);

    ThreadJoiner joiner(threads);

    Iter start = start_it;
    for (ul i = 0; i < (num_threads - 1); i++) {
        Iter end = start;
        std::advance(end, blk_sz);
        std::packaged_task<T(Iter, Iter)> task(func<Iter, T>);
        futures[i] = task.get_future();
        threads[i] = thread(std::move(task), start, end);
        start = end;
    }

    T result = init + func<Iter, T>(start, end_it);
    for (ul i = 0; i < num_threads - 1; i++) {
        result += futures[i].get();
    }

    return result;
}

void test() {
    srand(time(0));
    long long res = 0;
    vector<long long> arr;
    int arr_len = 1000000;
    for (int i = 0; i < arr_len; i++) {
        int ele = 5;
        arr.push_back(ele);
    }

    auto start_tim = std::chrono::system_clock::now();
    res = Accumulate(arr.begin(), arr.end(), res);
    auto end_tim = std::chrono::system_clock::now();
    auto dur = (end_tim - start_tim);
    int dur_ms = std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
    cout << "Parallel Result : " << res << " time taken " << dur_ms << endl;

    res = 0;
    start_tim = std::chrono::system_clock::now();
    for (int i = 0; i < arr_len; i++) {
        res += arr[i];
    }
    end_tim = std::chrono::system_clock::now();
    dur = (end_tim - start_tim);
    dur_ms = std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
    cout << "Serial Result : " << res << " time taken " << dur_ms << endl;
}

int main() {
    test();
}