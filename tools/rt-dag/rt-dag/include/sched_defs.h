#ifndef SCHED_DEFS_H_
#define SCHED_DEFS_H_

// sched_deadline syscall
#ifdef __x86_64__
#define __NR_sched_setattr		314
#define __NR_sched_getattr		315
#endif

#ifdef __i386__
#define __NR_sched_setattr		351
#define __NR_sched_getattr		352
#endif

#ifdef __arm__
#define __NR_sched_setattr		380
#define __NR_sched_getattr		381
#endif
#define sched_setattr(pid, attr, flags) syscall(__NR_sched_setattr, pid, attr, flags)
#define sched_getattr(pid, attr, size, flags) syscall(__NR_sched_getattr, pid, attr, size, flags)

typedef unsigned long long u64;
typedef unsigned int u32;
typedef int s32;

struct sched_attr {
    u32 size;

    u32 sched_policy;
    u64 sched_flags;

    /* SCHED_NORMAL, SCHED_BATCH */
    s32 sched_nice;

    /* SCHED_FIFO, SCHED_RR */
    u32 sched_priority;

    /* SCHED_DEADLINE */
    u64 sched_runtime;
    u64 sched_deadline;
    u64 sched_period;
};

#endif // SCHED_DEFS_H_
