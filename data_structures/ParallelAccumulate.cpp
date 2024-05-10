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

template <typename Iter, typename T>
void func(Iter blk_start, Iter blk_end, T& result) {
    result = std::accumulate(blk_start, blk_end, result);
}

template <typename Iter, typename T>
T Accumulate(Iter start_it, Iter end_it, T init) {
    unsigned long sz = std::distance(start_it, end_it);
    if (sz == 0) {
        // nothing to compute
        return init;
    }

    // minimum number of elements per thread.
    unsigned long mini_per_thread = 25;
    unsigned long num_threads = (sz + mini_per_thread - 1) / mini_per_thread;

    unsigned long hw_threads = std::thread::hardware_concurrency();
    hw_threads = hw_threads == 0 ? 2 : hw_threads; // in case hardware_concurrency returns 0

    unsigned long actual_threads = min(hw_threads, num_threads);
    unsigned long blk_size = sz / actual_threads;

    vector<T> results(actual_threads);
    vector<unique_ptr<thread> > threads(actual_threads - 1);

    Iter blk_start = start_it;

    for (unsigned long i = 0; i < (actual_threads - 1); i++) {
        Iter blk_end = blk_start;
        std::advance(blk_end, blk_size);
        threads[i] = make_unique<thread>(func<Iter, T>, blk_start, blk_end, std::ref(results[i]));
        blk_start = blk_end;
    }
    func(blk_start, end_it, results[actual_threads - 1]);

    for (auto& t: threads) {
        t->join();
    }

    return std::accumulate(results.begin(), results.end(), init);
}

void test() {
    srand(time(0));
    long long res = 0;
    vector<long long> arr;
    for (int i = 0; i < 10; i++) {
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
    for (int i = 0; i < 10; i++) {
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