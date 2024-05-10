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


template <typename T>
class ThreadSafeList {
    struct Node {
        shared_ptr<T> data;
        unique_ptr<Node> next;
        mutex mt;
    };

    Node head;

    public:

    ThreadSafeList() {}
    ThreadSafeList(const ThreadSafeList& other) = delete;
    ThreadSafeList& operator=(const ThreadSafeList& other) = delete;

    void push_front(T value) {
        shared_ptr<T> value_ptr = make_shared<T> (move(value));
        unique_ptr<Node> new_node = make_unique<Node>();
        new_node->data = value_ptr;

        lock_guard<mutex> lk(head.mt);
        new_node->next = std::move(head.next);
        head.next = std::move(new_node);
    }

    template <typename Function>
    void for_each(Function f) {
        unique_lock<mutex> lk(head.mt);
        Node* next = head.next.get();

        while (next != nullptr) {
            unique_lock<mutex> next_lk(next->mt);

            // All operations for the current node done till this point.
            lk.unlock();
            f(next->data);
            next = next->next.get();
            lk = std::move(next_lk);
        }
    }

    template <typename Predicate>
    shared_ptr<T> find_first(Predicate p) {
        unique_lock<mutex> lk(head.mt);
        Node* next = head.next.get();
        while (next != nullptr) {
            unique_lock<mutex> next_lk(next->mt);

            // All operations for the current node done till this point
            lk.unlock();
            if (p(*next->data)) {
                return next->data;
            }
            next = next->next.get();
            lk = std::move(next_lk);
        }

        return shared_ptr<T>();
    }

    template <typename Predicate>
    void remove_if(Predicate p) {
        unique_lock<mutex> lk(head.next);
        Node* prev_next = head.get();
        Node* next = head.next.get();
        while (next != nullptr) {
            unique_lock<mutex> next_lk(next->mt);
            
            if (p(*next->data)) {
                // This satisfies the condition. need to delete this node
                // Lock for previous node is still held.
                unique_ptr<Node> old_next = std::move(prev_next->next);
                prev_next->next = std::move(next->next);
                next_lk.unlock();
            } else {
                lk.unlock();
                prev_next = next;
                next = next->next.get();
                lk = std::move(next_lk);
            }
        }
    }
};

int main() {
    
}