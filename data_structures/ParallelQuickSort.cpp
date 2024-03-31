#include <iostream>
#include <thread>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <shared_mutex>
#include <future>

using namespace std;

int partition(vector<int>& arr, int start, int end) {
    int pivot = arr[end - 1];
    int start_pos = start;
    for (int i = start; i < end; i++) {
        if (arr[i] < pivot) {
            swap(arr[i], arr[start_pos]);
            start_pos++;
        }
    }

    swap(arr[end - 1], arr[start_pos]);
    return start_pos;
}

void quick_sort(vector<int>& arr, int start, int end) {
    if (start >= end) {
        return;
    }

    int pivot_ind = partition(arr, start, end);
    future<void> lower_half = std::async(quick_sort, std::ref(arr), start, pivot_ind);
    quick_sort(arr, pivot_ind + 1, end);
    lower_half.wait();
}

void test() {
    vector<int> arr = {10,7,8,9,1,5};
    quick_sort(arr, 0, arr.size());
    for (int ele: arr) cout << ele << " "; cout << endl;
}

int main() {
    test();
}