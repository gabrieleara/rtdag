#ifndef RTDAG_SCHEDUTILS_H
#define RTDAG_SCHEDUTILS_H

#include <chrono>

#include "newstuff/integers.h"

class sched_info {
public:
    using ns = std::chrono::duration<u64, std::nano>;

private:
    u32 _priority;
    ns _runtime;
    ns _deadline;
    ns _period;

public:
    // TODO: remove default constructor in the future
    sched_info() = default;

    sched_info(u32 priority, ns runtime, ns deadline, ns period);

    void set() const;

    u32 priority() const {
        return _priority;
    }

    ns runtime() const {
        return _runtime;
    }

    ns deadline() const {
        return _deadline;
    }

    ns period() const {
        return _period;
    }
};

#endif // RTDAG_SCHEDUTILS_H
