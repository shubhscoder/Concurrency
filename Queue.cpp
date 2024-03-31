#include <iostream>
#include <thread>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <vector>

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

class LeaderFollower {
    shared_ptr<CustomSemaphore> leader_arr_;
    shared_ptr<CustomSemaphore> follow_arr_;

    public:
    LeaderFollower() : leader_arr_(make_shared<CustomSemaphore>(0)),
    follow_arr_(make_shared<CustomSemaphore>(0)) {}

    void leader() {
        leader_arr_->signal();
        follow_arr_->wait(); // 5 leaders waiting
        // Atleast one leader and atleast one follower at this point
        printf("Leader leaving\n");
    }

    void follower() {
        follow_arr_->signal(); // 3 followers come, and after each follower, context switch 
        leader_arr_->wait();
        // Atleast one leader and atleast one follower at this point
        printf("Follower leaving\n");
    }
};

class ExclusiveLeaderFollower {
    int leaders_;
    int followers_;
    shared_ptr<CustomSemaphore> lead_mt_;
    shared_ptr<CustomSemaphore> follow_mt_;

    shared_ptr<CustomSemaphore> leader_q_;
    shared_ptr<CustomSemaphore> follower_q_;

    shared_ptr<CustomSemaphore> lead_done_;
    shared_ptr<CustomSemaphore> follow_done_;

    public:

    ExclusiveLeaderFollower() : leaders_(0),
    followers_(0),
    lead_mt_(make_shared<CustomSemaphore>(1)),
    follow_mt_(make_shared<CustomSemaphore>(1)),
    leader_q_(make_shared<CustomSemaphore>(0)),
    follower_q_(make_shared<CustomSemaphore>(0)),
    lead_done_(make_shared<CustomSemaphore>(0)),
    follow_done_(make_shared<CustomSemaphore>(0)) {}

    void dance() {}

    void leader2() {
        lead_mt_->wait();
        leader_q_->signal();
        follower_q_->wait();

        dance();

        lead_done_->signal();
        follow_done_->wait();
        lead_mt_->signal();
    }

    void follower2() {
        follow_mt_->wait();
        follower_q_->signal();
        leader_q_->wait();

        dance();

        follow_done_->signal();
        lead_done_->wait();
        follow_mt_->signal();
    }
};

void LeadFollowTest() {
    ExclusiveLeaderFollower test;
    vector<unique_ptr<thread> > threads;

    for (int i = 0; i < 5; i++) {
        threads.push_back(make_unique<thread>(&ExclusiveLeaderFollower::leader2, &test));
    }

    for (int i = 0; i < 5; i++) {
        threads.push_back(make_unique<thread>(&ExclusiveLeaderFollower::follower2, &test));
    }

    for (auto& t: threads) {
        t->join();
    }
}

void FollowLeadTest() {
    ExclusiveLeaderFollower test;
    vector<unique_ptr<thread> > threads;

    for (int i = 0; i < 5; i++) {
        threads.push_back(make_unique<thread>(&ExclusiveLeaderFollower::follower2, &test));
    }

    for (int i = 0; i < 5; i++) {
        threads.push_back(make_unique<thread>(&ExclusiveLeaderFollower::leader2, &test));
    }

    for (auto& t: threads) {
        t->join();
    }
}

void ExclusiveLeaderFollowerTest() {
    LeadFollowTest();
    FollowLeadTest();
}

void LeaderFollowerTest() {
    LeaderFollower test;
    thread lead(&LeaderFollower::leader, &test);
    thread follow(&LeaderFollower::follower, &test);

    lead.join();
    follow.join();
}

int main() {
    ExclusiveLeaderFollowerTest();
}