#ifndef RTDAG_MULTI_QUEUE_H
#define RTDAG_MULTI_QUEUE_H

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <vector>
#include <condition_variable>
#include <mutex>

#include "logging.h"
#include "newstuff/integers.h"

class MultiQueue {
public:
    using mask_type = u64;

private:
    // Mutex to lock to access the multi queue
    std::mutex mtx;

    // Bitmask of elements currently in the buffer (at most
    // std::numeric_limits<queue_mask_type>::digits supported)
    mask_type busy_mask = 0;

    // Message buffer
    std::vector<void *> elems;

    // Indicates the number of tasks waiting for the i-th
    // elem to free up (zero-initialized in the constructor)
    std::vector<int> waiting;

    // The consumer waits on this variable
    std::condition_variable cv_ready;

    // Used to wait for the destination elem to free up
    // (producers queue here)
    std::vector<std::condition_variable> cv_busy;

public:
    MultiQueue(int num_elems) :
        elems(num_elems), waiting(num_elems, 0), cv_busy(num_elems) {}

    // may block if the i-th elem is busy; returns 1 if all
    // elems have been pushed as input to target (so it has
    // been notified)
    inline int push(int i, void *elem) {
        const int bit = 1 << i;
        std::unique_lock<std::mutex> lock(mtx);

        while (busy_mask & bit) {
            waiting[i]++;
            LOG_DEBUG("push() suspending...\n");
            cv_busy[i].wait(lock);
            LOG_DEBUG("push() woken up...\n");
            waiting[i]--;
        }
        elems[i] = elem;
        busy_mask |= bit;
        if (busy_mask == ((u64(1) << elems.size()) - 1)) {
            // Original implementation used _signal, which
            // guarantees to unblock at least one of the
            // waiters...
            cv_ready.notify_one();
            // Notified
            return 1;
        }

        // No notification
        return 0;
    }

    // only unblock once all num_elems elems have been
    // popped
    inline void pop(void *dest[], int num_elems) {
        assert(num_elems == elems.size());

        std::unique_lock<std::mutex> lock(mtx);
        while (busy_mask != (u64(1) << num_elems) - 1) {
            LOG_DEBUG("pop() suspending (busy_mask=%lx)...\n", busy_mask);
            cv_ready.wait(lock);
            LOG_DEBUG("pop() woken up...\n");
        }

        if (dest != nullptr) {
            std::memcpy(dest, elems.data(), sizeof(*dest) * elems.size());
        }

        busy_mask = 0;
        for (int i = 0; i < num_elems; i++) {
            if (waiting[i] > 0) {
                cv_busy[i].notify_one();
            }
        }
    }
};

#endif // RTDAG_MULTI_QUEUE_H
