#ifndef RTDAG_MQUEUE_H
#define RTDAG_MQUEUE_H

#include <bitset>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "logging.h"
#include "newstuff/integers.h"

class MultiQueue {
public:
    // TODO: set the maximum number of tasks in CMake
    using mask_type = std::bitset<RTDAG_MAX_TASKS>;

private:
    // Mutex to lock to access the multi queue
    std::mutex mtx;

    // Bitmask representing all the tasks that have already
    // completed execution
    mask_type arrived_mask; // zero-initialized

    // Indicates the number of tasks waiting for the i-th
    // elem to free up (zero-initialized in the constructor)
    std::vector<int> waiting;

    // The consumer waits on this variable
    std::condition_variable cv_successor;

    // Used to wait for the destination elem to free up
    // (producers queue here)
    std::vector<std::condition_variable> cv_predecessors;

    // The pointers to the N vectors, one per task. Once initialized,
    // before the DAG execution, this information is CONSTANT (refers back
    // to each Edge)
    std::vector<void *> buffers;

    inline size_t num_predecessors() {
        return waiting.size();
    }

    // NOTICE: the lock MUST be held before calling this function
    inline bool all_arrived() {
        return arrived_mask.count() == num_predecessors();
    }

public:
    MultiQueue(size_t size) : waiting(size, 0), cv_predecessors(size) {
        if (size > arrived_mask.size()) {
            throw std::logic_error(
                "Exceeded maximum size for the multiqueue! Too many tasks!");
        }
    }

    // May block if the i-th elem is busy; returns 1 if all
    // elems have been pushed as input to target (so it has
    // been notified)
    inline bool push(size_t i) {
        if (i > waiting.size()) {
            throw std::logic_error("Accessed oob bit in MultiQueue!");
        }

        std::unique_lock<std::mutex> lock(mtx);

        while (arrived_mask.test(i)) {
            waiting[i]++;
            LOG_DEBUG("push() suspending...\n");
            cv_predecessors[i].wait(lock);
            LOG_DEBUG("push() woken up...\n");
            waiting[i]--;
        }

        arrived_mask.set(i);
        if (all_arrived()) {
            // Wakeup whoever is waiting
            cv_successor.notify_one();
            return true;
        }

        // No notification
        return false;
    }

    inline void pop() {
        std::unique_lock<std::mutex> lock(mtx);

        while (!all_arrived()) {
            LOG_DEBUG("pop() suspending...\n");
            cv_successor.wait(lock);
            LOG_DEBUG("pop() woken up...\n");
        }

        arrived_mask.reset();

        // Well done, now wake up everyone
        for (size_t i = 0; i < waiting.size(); ++i) {
            if (waiting[i] > 0) {
                cv_predecessors[i].notify_one();
            }
        }
    }

    void set_buffer(size_t index, void *buffer) {
        if (index > waiting.size()) {
            throw std::logic_error("Accessed oob bit in MultiQueue!");
        }

        buffers[index] = buffer;
    }
};

struct Edge {
    const int from;
    const int to;
    const int push_idx;
    MultiQueue &mq;
    std::vector<u8> msg;

    template <class Value>
    Value &as_value() {
        return *(reinterpret_cast<Value *>(msg.data()));
    }

    // The last argument is to differentiate with the other constructor
    template <class Value>
    Edge(MultiQueue &mq, int from, int to, int push_idx, const Value &value,
         bool unused) :
        from(from), to(to), push_idx(push_idx), mq(mq), msg(sizeof(Value)) {

        (void)unused;

        // Assign value to the buffer pointed by the vector
        as_value<Value>() = value;

        set_buffer();
    }

    Edge(MultiQueue &mq, int from, int to, int push_idx, int msg_size) :
        from(from), to(to), push_idx(push_idx), mq(mq), msg(msg_size, '.') {

        // The message is initialized with '.' (above) and a termination
        // std::string character. This is to avoid errors when checking
        // that the transferred data is correct.
        msg[msg_size - 1] = '\0';

        set_buffer();
    }

private:
    void set_buffer() {
        mq.set_buffer(push_idx, msg.data());
    }
};

#endif // RTDAG_MQUEUE_H
