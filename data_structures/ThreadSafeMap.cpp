#include <iostream>
#include <stack>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <exception>
#include <unordered_map>
#include <list> 
#include <shared_mutex>

using namespace std;

const int default_num_buckets = 99991;

template <typename KEY, typename VALUE>
class CoarseThreadSafeMap {
    unordered_map<KEY, VALUE> mp_;
    std::shared_mutex mp_mt_;

    public:

    shared_ptr<VALUE> get(KEY key) {
        shared_ptr<VALUE> result;
        shared_lock<std::shared_mutex> lk(mp_mt_);
        if (mp_.find(key) != mp_.end()) {
            result = make_shared<VALUE>(mp_[key]);
        }

        return result;
    }

    void put(KEY key, VALUE value) {
        lock_guard<std::shared_mutex> lk(mp_mt_);
        mp_[key] = value;
    }
};


template<typename KEY, typename VALUE> 
class Bucket {
    list<pair<KEY, VALUE> > lst_;
    shared_mutex lst_mt_;

    typedef typename list<pair<KEY, VALUE> >::iterator iterator;
    // Function supposed to be called with lock held.
    iterator getKey(KEY key) {
        std::find_if(lst_.begin(), lst_.end(), [&](pair<KEY, VALUE> item) {
            return item.first == key;
        });
    }

    public:

    Bucket() {}

    void addOrUpdate(KEY key, VALUE value) {
        lock_guard<shared_mutex> lk(lst_mt_);
        auto it = getKey(key);
        if (it != end(lst_)) {
            it->second = value;
        } else {
            lst_.push_back({key, value});
        }
    }

    bool deleteKey(KEY key) {
        lock_guard<shared_mutex> lk(lst_mt_);
        bool found = false;
        auto it = getKey(key);

        if (it != end(lst_)) {
            lst_.erase(it);
            found = true;
        }

        return found;
    }

    shared_ptr<VALUE> getValue(KEY key) {
        shared_lock<shared_mutex> lk(lst_mt_);
        auto it = getKey(key);
        shared_ptr<VALUE> result;
        if (it != end(lst_)) {
            result = make_shared<VALUE> (it->second);
        }

        return result;
    }
};

template<typename KEY, typename VALUE>
class ThreadSafeMap {
    int n_buckets_;
    vector<unique_ptr<Bucket<KEY, VALUE> > > buckets_;
    private:

    size_t getIndex(KEY key) {
        return (hash<KEY> {} (key)) % n_buckets_;
    }

    public:
    
    ThreadSafeMap(int n_buckets = default_num_buckets) : n_buckets_(n_buckets) {
        for (int i = 0; i < n_buckets_; i++) {
            buckets_.push_back(make_unique<Bucket<KEY, VALUE> >());
        }
    }

    void put(KEY key, VALUE value) {
        buckets_[getIndex(key)]->addOrUpdate(key, value);
    }

    shared_ptr<VALUE> get(KEY key, VALUE value) {
        return buckets_[getIndex(key)]->getValue(key);
    }
    
    bool deleteKey(KEY key) {
        return buckets_[getIndex(key)]->deleteKey(key);
    }
};

int main() {

}