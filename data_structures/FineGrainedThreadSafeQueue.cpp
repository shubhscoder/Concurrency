#include <iostream>
#include <stack>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <exception>
#include <queue>

using namespace std;

template <typename T>
class SingleThreadedQueue {
    private:

    struct Node {
        T data;
        unique_ptr<Node> next;

        Node(T data_) : data(std::move(data)) {}
    };
    unique_ptr<Node> head_;
    Node* tail_;

    public:

    SingleThreadedQueue() {}
    SingleThreadedQueue(const SingleThreadedQueue& other) = delete;
    SingleThreadedQueue& operator=(const SingleThreadedQueue& other) = delete;

    shared_ptr<T> TryPop() {
        if (!head_) {
            // no elements in the queue
            return shared_ptr<T>();
        }

        shared_ptr<T> val = make_shared<T> (move(head_->data));
        unique_ptr<Node> old_head = move(head_);
        head_ = move(old_head->next);
        return val; 
    }

    void Push(T new_val) {
        unique_ptr<Node> new_node = make_unique<Node> (move(new_val));
        Node* new_tail = new_node.get();
        if (tail_) {
            tail_->next = move(new_node);
        } else {
            head_ = move(new_node);
        }
        
        tail_ = new_tail;
    }
};

template <typename T>
class EfficientSingleThreadedQueue {
    private:
    struct Node {
        shared_ptr<T> data;
        unique_ptr<Node> next;
    };

    unique_ptr<Node> head_;
    Node* tail_;

    public:
    EfficientSingleThreadedQueue() : head_(new Node()), tail_(head_.get()) {}
    EfficientSingleThreadedQueue(const EfficientSingleThreadedQueue& other) = delete;
    EfficientSingleThreadedQueue& operator=(const EfficientSingleThreadedQueue& other) = delete;

    shared_ptr<T> TryPop() {
        // Only place where the tail is accessed. Nothing modified as well.
        if (head_.get() == tail_) {
            // only the dummy node present
            return shared_ptr<T>();
        }

        shared_ptr<T> result = head_->data;
        // head = head->next
        unique_ptr<Node> old = move(head_);
        head_ = move(old->next);
        return result;
    }

    shared_ptr<T> Push(T new_val) {
        shared_ptr<T> data_ptr = make_shared<T>(move(new_val));
        unique_ptr<Node> new_dummy = make_unique<Node>();
        Node* new_tail = new_dummy.get();
        tail_->data = data_ptr;
        tail_->next = move(new_dummy);
        tail_ = new_tail;
        // tail->data = data_ptr
        // tail->next = new dummy
    }
};

/*

Analysis of the code:

1. Is all shared data protected and all invariants okay?
    a. tail_ -> Yes
    b. head_ -> Yes
   Not in constructor, but we assume that the data structure is accessed only after the construction is
   complete.
2. Are there any race conditions inherent in the interface?
   No, all operations enclosed
3. Exceptions?
   a. Line 140? Not a problem, no data modified.
   b. Line 148? Not a problem, no data modified.
   c. In Push exceptions can occur till line 158, and no data is modified till that time.
4. Deadlock? Does not look like it.
*/
template <typename T>
class ThreadSafeQueueMinimal {
    private:
    struct Node {
        shared_ptr<T> data;
        unique_ptr<Node> next;
    };

    unique_ptr<Node> head_;
    mutex head_mt_;

    Node* tail_;
    mutex tail_mt_;

    public:
    
    ThreadSafeQueueMinimal() : head_(new Node()), tail_(head_.get()) {}
    ThreadSafeQueueMinimal(const ThreadSafeQueueMinimal& other) = delete;
    ThreadSafeQueueMinimal& operator=(const ThreadSafeQueueMinimal& other) = delete;

    shared_ptr<T> TryPop() {
        Node* cur_tail;
        lock_guard<mutex> lk(head_mt_);
        {
            lock_guard<mutex> lk(tail_mt_);
            cur_tail = tail_;
        }

        if (head_.get() == cur_tail) {
            // Only the dummy node present.
            return shared_ptr<T>();
        }

        shared_ptr<T> result = head_->data;
        unique_ptr<Node> old = move(head_);
        head_ = move(old->next);
        return result;
    }

    shared_ptr<T> Push(T new_val) {
        shared_ptr<T> data_ptr = make_shared<T>(new_val);
        unique_ptr<Node> new_dummy = make_unique<Node>();
        Node* new_tail = new_dummy.get();
        {
            lock_guard<mutex> lk(tail_mt_);
            tail_->data = data_ptr;
            tail_->next = move(new_dummy);
            tail_ = new_tail;
        }
    }
};


/*

Analysis of the code:

1. Is all shared data protected and all invariants okay?
    a. tail_ -> Yes
    b. head_ -> Yes
   Not in constructor, but we assume that the data structure is accessed only after the construction is
   complete.
2. Are there any race conditions inherent in the interface?
   No, all operations enclosed
3. Exceptions?
   a. Line 140? Not a problem, no data modified.
   b. Line 148? Not a problem, no data modified.
   c. In Push exceptions can occur till line 158, and no data is modified till that time.
4. Deadlock? Does not look like it.

If this method signature is implemented:

bool TryPop(T& val) {

}

OR this

void WaitAndPop(T& val) {

}

Somewhere down the line, we would copy data to val, which can CREATE AN EXCEPTION
HENCE EXCEPTION SAFETY CAN BE COMPRIMISED.

*/

template <typename T>
class ThreadSafeQueue {
private:
    struct Node {
        shared_ptr<T> data;
        unique_ptr<Node> next;
    };

    unique_ptr<Node> head_;
    mutex head_mt_;

    Node* tail_;
    mutex tail_mt_;

    int sz_;
    mutex sz_mt_;

    condition_variable cnd_;

    Node* getTail() {
        lock_guard<mutex> lk(tail_mt_);
        return tail_;
    }

    void updateSize(int add) {
        lock_guard<mutex> lk(sz_mt_);
        sz_ += add;
    }

    public:

    ThreadSafeQueue() : head_(new Node()), tail_(head_.get()), sz_(0) {}
    ThreadSafeQueue(const ThreadSafeQueue& other) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue& other) = delete;

    shared_ptr<T> TryPop() {
        Node* cur_tail;
        lock_guard<mutex> lk(head_mt_);
        {
            lock_guard<mutex> lk(tail_mt_);
            cur_tail = tail_;
        }

        if (head_.get() == cur_tail) {
            // Only the dummy node present.
            return shared_ptr<T>();
        }

        updateSize(-1);
        shared_ptr<T> result = head_->data;
        unique_ptr<Node> old = std::move(head_);
        head_ = move(old->next);
        return result;
    }

    void Push(T new_val) {
        shared_ptr<T> data_ptr = make_shared<T>(new_val);
        unique_ptr<Node> new_dummy = make_unique<Node>();
        Node* new_tail = new_dummy.get();
        {
            lock_guard<mutex> lk(tail_mt_);
            tail_->data = data_ptr;
            tail_->next = std::move(new_dummy);
            tail_ = new_tail;
        }
        updateSize(1);
        cnd_.notify_one();
    }

    shared_ptr<T> WaitAndPop() {
        unique_lock<mutex> lk(head_mt_);
        cnd_.wait(lk, [this] () { return head_.get() != getTail();});
        updateSize(-1);
        shared_ptr<T> result = head_->data;
        unique_ptr<Node> old = std::move(head_);
        head_ = std::move(old->next);
        return result;
    }

    int sz() {
        lock_guard<mutex> lk(sz_mt_);
        return sz_;
    }
};

template <typename T>
void runThread(shared_ptr<ThreadSafeQueue<T>> q, int adds, int removs) {
    for (int i = 0; i < adds; i++) {
        q->Push(rand() % 10);
    }

    while (removs--) {
        auto val = q->WaitAndPop();
        assert(val != nullptr);
    }
}

void testThreadSafeQueue() {
    auto q = make_shared<ThreadSafeQueue<int> > ();
    int add1 = rand() % 10 + 2;
    int removs1 = rand() % add1;

    int add2 = rand() % 10 + 2;
    int removs2 = rand() % 10 % add2;

    thread t1(runThread<int>, q, add1, removs1);
    thread t2(runThread<int>, q, add2, removs2);

    t1.join();
    t2.join();

    assert(q->sz() == (add1 + add2 - removs1 - removs2));
    cout << "Test passed " << endl;
}

int main() {
    testThreadSafeQueue();
}