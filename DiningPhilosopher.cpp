#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>

using namespace std;

// class CustomSemaphore {
//     int cnt_;
//     mutex cnt_mt_;
//     condition_variable cnt_cond_;

//     public:
//     CustomSemaphore(int cnt) : cnt_(cnt) {}

//     void wait() {
//         std::unique_lock<mutex> lk(cnt_mt_);
//         cnt_cond_.wait(lk, [&](){ return cnt_ > 0;});
//         cnt_--;
//     }

//     void signal() {
//         std::lock_guard<mutex> lk(cnt_mt_);
//         cnt_++;
//         cnt_cond_.notify_one();
//     }
// };

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

class DiningPhilosopher {
    int n_;
    vector<shared_ptr<CustomSemaphore> > forks_;

    // allow only 4 semaphores at the table to prevent deadlock
    shared_ptr<CustomSemaphore> table_limit_;

    public:

    DiningPhilosopher() : n_(5),
                        
                          table_limit_(make_shared<CustomSemaphore>(n_ - 1)) {
        for(int i = 0; i < n_; i++){
            forks_.push_back(make_shared<CustomSemaphore>(1));
        }
    }

    void fck() {}
    void eat() {}

    int getleft(int id) {
        return id;
    }

    int getright(int id) {
        return (id + 1) % 5;
    }

    /*

    Conditions:
    • Only one philosopher can hold a fork at a time.
    • It must be impossible for a deadlock to occur.
    • It must be impossible for a philosopher to starve waiting for a fork.
    • It must be possible for more than one philosopher to eat at the same time.

    void get_forks(int philo_id) {
        forks_[getright(philo_id)]->wait();
        forks_[getleft(philo_id)]->wait();
    }

    void put_forks(int philo_id) {
        forks_[getright(philo_id)]->signal();
        forks_[getleft(philo_id)]->signal();
    }

    Problem : This solution clearly has a deadlock.
    Each person holds to the right fork, gets context switched,
    next person holds to the right one. 
    Because the table is circular, eventually everyone is holding one fork.

    Solution : We limit number of people on table at a time to 4. How? Using a semaphore,
    with prefilled value set to 4. 

    Can this cause starvation? 

    Weak semaphore : Semaphore will wake up one of the waiting threads.
    At a given time there can be one 1 thread waiting. It is guaranteed that
    this thread will be waken up. Therefore, no starvation.

    */

    void get_forks(int philo_id) {
        // check if there is room on the table.
        table_limit_->wait();
        forks_[getright(philo_id)]->wait();
        cout<<"Got right for "<<philo_id<<endl;
        forks_[getleft(philo_id)]->wait();
        cout<<"Got left for "<<philo_id<<endl;
    }

    void put_forks(int philo_id) {
        forks_[getleft(philo_id)]->signal();
        forks_[getright(philo_id)]->signal();
        // leaving table so decrement cnt.
        table_limit_->signal();
    }

    void Philosopher(int id) {
        int n = 1;
        while (n--) {
            fck();
            // if (id == 0) 
            // cout << "waiting to get forks\n";
            get_forks(id);
            cout << "Got forks "<<id<<endl;
            eat();
            put_forks(id);
        }
    }
};

void TestDiningPhilosopher() {
    vector<unique_ptr<thread> > threads;
    DiningPhilosopher test;
    for (int i = 0; i < 5; i++) {
        threads.push_back(make_unique<thread>(&DiningPhilosopher::Philosopher, &test, i));
    }

    for (auto& t: threads) {
        t->join();
    }
}

class DiningPhilosopherTanenbaum {
    vector<string> state_;
    shared_ptr<CustomSemaphore> mt_;
    vector<shared_ptr<CustomSemaphore> > sems_;
    int n_;

    public:
    DiningPhilosopherTanenbaum() : n_(5), state_(vector<string>(5, "thinking")),
                                   mt_(make_shared<CustomSemaphore>(1)) {

        for (int i = 0; i < 5; i++) {
            sems_.push_back(make_shared<CustomSemaphore>(0));
        }
    }
    int getleft(int id) {
        return (id + 1) % n_;
    }

    int getright(int id) {
        return id;
    }

    void test(int id) {
        if (state_[id] == "hungry" && state_[getleft(id)] != "eating" && state_[getright(id)] != "eating") {
            state_[id] = "eating";
            sems_[id]->signal();
        }
    }

    void get_forks(int id) {
        mt_->wait();
        state_[id] = "hungry";
        test(id);
        mt_->signal();
        sems_[id]->wait();
    }

    void put_forks(int id) {
        mt_->wait();
        state_[id] = "thinking";
        test(getright(id));
        test(getleft(id));
        mt_->signal();
    }

    void thinking(){}
    void eating(){}

    void philosopher(int id) {
        int n = 5;
        while (n--)
        {
            thinking();
            get_forks(id);
            eating();
            put_forks(id);
        }
    }
};

void TestDiningPhilosopherTanenbaum() {
    vector<unique_ptr<thread> > threads;
    DiningPhilosopherTanenbaum test;
    for (int i = 0; i < 5; i++) {
        threads.push_back(make_unique<thread>(&DiningPhilosopherTanenbaum::philosopher, &test, i));
    }

    for (auto& t: threads) {
        t->join();
    }
}

int main() {
    TestDiningPhilosopher();
}