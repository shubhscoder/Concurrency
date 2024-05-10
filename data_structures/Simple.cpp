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
using Tp = std::chrono::time_point<std::chrono::steady_clock>;

void test() {
    set<Tp> points;
    points.insert(std::chrono::steady_clock::now() + std::chrono::seconds(5));
    
    while (true) {
        auto cur = std::chrono::steady_clock::now();
        if (*points.begin() < cur) {
            cout << "Hello\n";
            auto tp2 = *points.begin();
            points.erase(points.begin());
            points.insert(cur + std::chrono::seconds(5));
        }
    }
}

int main() {
    test();
}