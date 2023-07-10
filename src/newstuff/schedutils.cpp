#include "schedutils.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "../logging.h"
#include "../sched_defs.h"

sched_info::sched_info(u32 priority, sched_info::ns runtime,
                       sched_info::ns deadline, sched_info::ns period) :
    _priority(priority),
    _runtime(runtime),
    _deadline(deadline),
    _period(period) {

    // Priority wins over deadline in this implementation
    if (_priority > 0) {
        return;
    }

    if (_runtime > _deadline) {
        LOG(ERROR,
            "invalid scheduling parameters: runtime %lu > deadline "
            "%lu.\n",
            _runtime.count(), _deadline.count());
        std::exit(EXIT_FAILURE);
    }

    if (_deadline > _period) {
        LOG(ERROR,
            "invalid scheduling parameters: deadline %lu > period %lu.\n",
            _deadline.count(), _period.count());
        std::exit(EXIT_FAILURE);
    }
}

void sched_info::set() const {
    struct sched_attr sa;
    if (sched_getattr(0, &sa, sizeof(sa), 0) < 0) {
        LOG(ERROR, "sched_getattr() failed: %s.\n", std::strerror(errno));
        std::exit(EXIT_FAILURE);
    }

    // Use RT priority if set
    if (_priority > 0) {
        sa.sched_policy = SCHED_FIFO;
        sa.sched_priority = _priority;
    } else {
        // FIXME: for now we are using the deadline for everything!!!!
        sa.sched_policy = SCHED_DEADLINE;
        sa.sched_runtime = _deadline.count();
        sa.sched_deadline = _deadline.count();
        sa.sched_period = _deadline.count();
    }

    if (sched_setattr(0, &sa, 0) < 0) {
        LOG(ERROR, "sched_setattr() failed: %s.\n", std::strerror(errno));
        LOG(ERROR, "parameters: P=%d DL_C=%lu DL_D=%lu DL_T=%lu\n",
            sa.sched_priority, sa.sched_runtime, sa.sched_deadline,
            sa.sched_period);
        LOG(ERROR,
            "make sure you can run real-time tasks, for example by \n"
            "         running the following command before executing rtdag:\n"
            "           echo -1 | sudo tee "
            "/proc/sys/kernel/sched_rt_runtime_us\n");
        std::exit(EXIT_FAILURE);
    }
}
