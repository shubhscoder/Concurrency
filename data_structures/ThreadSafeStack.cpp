#include <iostream>
#include <stack>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <exception>

using namespace std;

/*
1. First, basic thread safety can be ensured as each member is protected with a mutex
2. There is still a race condition between empty and Pop, however, that is not problematic as 
   the pop checks after holding the lock that the stack is not empty
3. Locking a mutex may throw exception, but that is rare, and no data has been modified when the
   exception is thrown, so good. Pop code can throw an exception, but again no data has been 
   modified.
4. Construting destructing must happen only once, so user of the stack object should ensure
   that the object is fully constructed before being called and access to a partially 
   constructed object should not be permitted.

Performance:

Only one thread is essentially doing work at a time. This serialization of threads can 
limit the prformance of the application.
*/


template <typename T>
class ThreadSafeStack {
    stack<T> st_;
    mutex st_mt_;
    condition_variable st_cnd_;

    public:

    ThreadSafeStack() {} // default
    ThreadSafeStack(const ThreadSafeStack& other) {
        lock_guard<mutex> lk(st_mt_);
        st_ = other.st_;
    }
    ThreadSafeStack& operator=(const ThreadSafeStack&) = delete;

    void Push(T value) {
        lock_guard<mutex> lk(st_mt_);
        st_.push(value);
        st_cnd_.notify_one();
    }

    shared_ptr<T> Pop() {
        lock_guard<mutex> lk(st_mt_);
        shared_ptr<T> val;
        if (!st_.empty()) {
            val = make_shared<T>(st_.top());
            st_.pop();
        }

        return val;
    }

    void Pop(T& value) {
        lock_guard<mutex> lk(st_mt_);
        if (st_.empty()) throw std::exception();
        value = st_.top();
        st_.pop();
    }

    void WaitAndPop(T& value) {
        unique_lock<mutex> lk(st_mt_);
        st_cnd_.wait(lk, [this] () { return !st_.empty(); });
        value = st_.top();
        st_.pop();
    }

    bool empty() {
        lock_guard<mutex> lk(st_mt_);
        return st_.empty();
    }

    int sz() {
        lock_guard<mutex> lk(st_mt_);
        return st_.size();
    }
};

template<typename T>
void threadFn(shared_ptr<ThreadSafeStack<T> > st, int adds, int removs) {
    for (int i = 0; i < adds; i++) {
        st->Push(rand());
    }

    for (int i = 0; i < removs; i++) {
        T value;
        st->WaitAndPop(value);
    }
}

void test() {
    srand(time(0));
    shared_ptr<ThreadSafeStack<int> > st = make_shared<ThreadSafeStack<int> > ();
    int add1 = rand() % 10 + 2;
    int removs1 = rand() % add1;

    int add2 = rand() % 10 + 2;
    int removs2 = rand() % 10 % add2;

    thread t1(threadFn<int>, st, add1, removs1);
    thread t2(threadFn<int>, st, add2, removs2);

    t1.join();
    t2.join();

    assert(st->sz() == (add1 + add2 - removs1 - removs2));
}


int main() {
    test();
}