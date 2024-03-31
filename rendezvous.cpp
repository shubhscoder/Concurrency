/*
Puzzle: Generalize the signal pattern so that it works both ways. Thread A has
to wait for Thread B and vice versa. In other words, given this code
Thread A
1 statement a1
2 statement a2
Thread B
1 statement b1
2 statement b2
we want to guarantee that a1 happens before b2 and b1 happens before a2. In
writing your solution, be sure to specify the names and initial values of your
semaphores (little hint there).

*/

#include <iostream>
#include <thread>
#include <memory>
#include <condition_variable>
#include <mutex>

using namespace std;

class CustomSemaphore {
    int cnt_;
    std::mutex cnt_mut_;
    std::condition_variable cnt_cnd_;

    public:
    CustomSemaphore(int cnt) : cnt_(cnt) {}

    void signal() {
       std::lock_guard<std::mutex> lk(cnt_mut_);
       cnt_++;
       cnt_cnd_.notify_one();
    }

    void wait() {
        std::unique_lock<std::mutex> lk(cnt_mut_);
        cnt_cnd_.wait(lk, [this](){return cnt_ > 0;}); // If the condition holds true, then waiting is done.
        cnt_--;
    }
};

class Rendezvous {
    shared_ptr<CustomSemaphore> a1b2;
    shared_ptr<CustomSemaphore> b1a2;

    public:

    Rendezvous() : a1b2(make_shared<CustomSemaphore>(0)),
                   b1a2(make_shared<CustomSemaphore>(0)) {}

    void ThreadA() {
        cout << "Statement a1" << endl;
        a1b2->signal();
        b1a2->wait();
        cout << "Statement a2" << endl;
    }

    void ThreadB() {
        cout << "Statement b1" << endl;
        b1a2->signal();
        a1b2->wait();
        cout << "Statement b2" << endl;
    }

    /*
    Interesting notes:
    A:
    a1b2->signal();
    b1a2->wait();

    B:
    a1b2->wait();
    b1a2->signal();

    Also works. But it may require an additional switch depending on who comes first.
    And hence is less efficient.

    A:
    b1a2->wait();
    a1b2->signal();

    B:
    a1b2->wait();
    b1a2->signal();

    This will cause a deadlock.
    */
};

int main() {
    Rendezvous test;

    thread A(&Rendezvous::ThreadA, &test);
    thread B(&Rendezvous::ThreadB, &test);

    A.join();
    B.join();
}

/*
Outputs:

Run 1:
Statement a1
Statement a2
Statement b1
Statement b2

Run 2:
Statement b1
Statement a1
Statement a2
Statement b2

Run 3:
Statement b1
Statement b2
Statement a1
Statement a2

After adding synchronization.

a1 comes before b2.
b1 comes before a2.

Rest they can be merged anyhow and that is what is seen by running the program multiple times.
*/