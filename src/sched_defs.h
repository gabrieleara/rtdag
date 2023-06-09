#ifndef SCHED_DEFS_H_
#define SCHED_DEFS_H_

#include <sched.h>       /* Definition of SCHED_* constants */
#include <sys/syscall.h> /* Definition of SYS_* constants */
#include <unistd.h>
#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef int32_t s32;

struct sched_attr {
    u32 size;
    u32 sched_policy = 0;   /* Policy (SCHED_*) */
    u64 sched_flags = 0;    /* Flags */
    s32 sched_nice = 0;     /* Nice value (SCHED_OTHER, SCHED_BATCH) */
    u32 sched_priority = 0; /* Static priority (SCHED_FIFO, SCHED_RR) */

    /* Remaining fields are for SCHED_DEADLINE */
    u64 sched_runtime = 0;
    u64 sched_deadline = 0;
    u64 sched_period = 0;
};

int sched_setattr(pid_t pid, sched_attr *attr, unsigned int flags) {
    return syscall(SYS_sched_setattr, pid, attr, flags);
}

int sched_getattr(pid_t pid, sched_attr *attr, unsigned int size, unsigned int flags) {
    return syscall(SYS_sched_getattr, pid, attr, size, flags);
}

#endif // SCHED_DEFS_H_
