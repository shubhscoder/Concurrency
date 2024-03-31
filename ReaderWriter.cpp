#include <iostream>
#include <thread>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <shared_mutex>

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

template <typename T>
class ReaderWriter {
    int reader_cnt_;
    shared_ptr<CustomSemaphore> reader_cnt_mt_;
    shared_ptr<CustomSemaphore> writer_mt_;

    public:
    ReaderWriter() : reader_cnt_(0),
                     reader_cnt_mt_(make_shared<CustomSemaphore>(1)),
                     writer_mt_(make_shared<CustomSemaphore>(1)) {}

    void writer() {
        writer_mt_.wait();
        // Writer Critical section.
        writer_mt_.signal();
    } 

    void reader() {
        reader_cnt_mt_.wait();
        reader_cnt_++;
        if (reader_cnt_ == 1) {
            // first reader
            writer_mt_.wait();
        }
        reader_cnt_mt_.signal();

        // Reader critical section/
        // note multiple readers can come here.

        reader_cnt_mt_.wait();
        reader_cnt_--;
        if (reader_cnt_ == 0) {
            // last one to leave, can unlock writer mt
            writer_mt_.signal();
        }
        reader_cnt_mt_.signal();
    }
};

/*
Problem with the above code is that, assume that a reader comes, 
it will let other readers come. 

Now last reader leaves.
Writer has a chance, but some reader comes again.
Clearly the writer is starving now.

Modify the solution such that when a writer is waiting, no reader should enter, Obviously after all readers have completed and exited.
Else that will cause deadlock.
*/

/*
How to do that?
When a writer is waiting, reader should check that.
How? Create a 
*/

template <typename T>
class ReaderWriterNoStarvation {
    int reader_cnt_;
    shared_ptr<CustomSemaphore> reader_cnt_mt_;
    shared_ptr<CustomSemaphore> writer_mt_;
    shared_ptr<CustomSemaphore> turns_tile_;

    public:
    ReaderWriterNoStarvation() : reader_cnt_(0),
                     reader_cnt_mt_(make_shared<CustomSemaphore>(1)),
                     writer_mt_(make_shared<CustomSemaphore>(1)),
                     turns_tile_(make_shared<CustomSemaphore>(1)) {}

    void writer() {
        turns_tile_->wait();
        writer_mt_.wait();
        // Writer Critical section.
        turns_tile_->signal();
        writer_mt_.signal();
    } 

    void reader() {
        turns_tile_->wait();
        turns_tile_->signal();

        reader_cnt_mt_.wait();
        reader_cnt_++;
        if (reader_cnt_ == 1) {
            // first reader
            writer_mt_.wait(); // locked only once, so no issues, and thus allows subsequent readers.
        }
        reader_cnt_mt_.signal(); // allows multiple readers.

        // Reader critical section/
        // note multiple readers can come here.

        reader_cnt_mt_.wait();
        reader_cnt_--;
        if (reader_cnt_ == 0) {
            // last one to leave, can unlock writer mt
            writer_mt_.signal();
        }
        reader_cnt_mt_.signal();
    }
};


class LightSwitch {
    int cnt_;
    shared_ptr<CustomSemaphore> mt_;

    public:

    LightSwitch() : mt_(make_shared<CustomSemaphore>(1)) {}

    void lock(shared_ptr<CustomSemaphore> sem) {
        mt_->wait();
        cnt_++;
        if (cnt_ == 1) {
            sem->wait();
        }
        mt_->signal();
    }

    void unlock(shared_ptr<CustomSemaphore> sem) {
        mt_->wait();
        cnt_--;
        if (cnt_ == 0) {
            sem->wait();
        }
        mt_->signal();
    }
};

/*
Write a solution to the readers-writers problem that gives priority
to writers. That is, once a writer arrives, no readers should be allowed to enter
until all writers have left the system.
*/

class ReaderWriterWithWriterPriority {
    shared_ptr<CustomSemaphore> any_;
    shared_ptr<CustomSemaphore> writer_;
    LightSwitch lk_;

    public:
    ReaderWriterWithWriterPriority() : any_(make_shared<CustomSemaphore>(1)) {}
    
    void Reader() {
        lk_.lock(any_);
        // Reader logic code.
        lk_.unlock(any_);
    }

    void Writer() {
        writer_->signal();
        any_->wait();
        // Writer logic code.
        any_->signal();
   }
};

class PriorityReaderWrite {
    // Signal from this would indicate that there is no reader in the CS.
    shared_ptr<CustomSemaphore> no_reader_;
    // Signal from this would indicate that there is no write in the CS.
    shared_ptr<CustomSemaphore> no_writer_;
    int writer_cnt_;
    shared_ptr<CustomSemaphore> writer_cnt_mt_;

    int reader_cnt_;
    shared_ptr<CustomSemaphore> reader_cnt_mt_;

    shared_ptr<CustomSemaphore> no_writer_;
    shared_ptr<CustomSemaphore> no_reader_;

    public:
    PriorityReaderWrite() : no_reader_(make_shared<CustomSemaphore>(1)),
                            no_writer_(make_shared<CustomSemaphore>(1)),
                            writer_cnt_mt_(make_shared<CustomSemaphore>(0)),
                            reader_cnt_mt_(make_shared<CustomSemaphore>(0)),
                            writer_cnt_(0),
                            reader_cnt_(0) {}

    void reader() {
        // make sure that there is not writer.
        // sufficient for the first writer to do it.

    }

    void writer() {
        // need to ensure that there is no reader. 
        // sufficient if only the first write does that.
        writer_cnt_mt_->wait();
        writer_cnt_++;
        if (writer_cnt_ == 1) {
            no_reader_->wait(); // no reader can enter now.
        }
        writer_cnt_mt_->signal();

        no_writer_->wait();
        // Critical section for the writer.
        no_writer_->signal();

        writer_cnt_mt_->wait();
        writer_cnt_--;
        if (writer_cnt_ == 0) {
            // last leaving writer makes room for readers.
            no_reader_->signal();
        }
        writer_cnt_mt_->signal();
    }

    void reader() {
        no_reader_->wait(); // this ensured that if a writer was waiting, the reader won't enter again.
        reader_cnt_mt_->wait();
        reader_cnt_++;
        if (reader_cnt_ == 1) {
            no_writer_->wait(); // no writer can enter now.
            reader_cnt_mt_->signal();
        }
        no_reader_->signal(); // IMPORTANT TO DO THIS BEFORE CS TO ALLOW MULTIPLE READERS.
        // Reader critical section

        reader_cnt_mt_->wait();
        reader_cnt_--;
        if (reader_cnt_ == 0) {
            no_writer_->signal();
        }
        reader_cnt_mt_->signal();
    }
};

/*
 * Easy explanation:

 When the writer is in the critical section, no reader should be present
 no writer should be presnet.

 Therefore it simply holds both the locks.
 However, we need only one writer to lock and rest writers can follow, because
 priority is for writer. i.e if writer is waiting no reader should read.

 Therefore we use Lightswitch with no reader as the semaphore.


 For readers:

 No writer should be present. Therefore it should hold the no writer lock in the CS
 However, it should not hold no_reader as we want to allow multiple readers.

 Therefore, in reader, first ensure that no_reader is available (Writer has not locked)
 Then lock the no writer, because the reader is now about to enter CS.

 Release the no reader lock allowing multiple readers.

 Execute cs
 Release  the no writer lock usign lightswitch.
*/

class ReaderWriterSTL {
    int cnt_;
    shared_mutex mt_;
    public:
    ReaderWriterSTL(int cnt) : cnt_(cnt) {}

    int Reader() {
        shared_lock<shared_mutex> lk(mt_);
        return cnt_;
    }

    void Writer() {
        lock_guard<shared_mutex> lk(mt_);
        cnt_++;
    }
};

int main() {

}