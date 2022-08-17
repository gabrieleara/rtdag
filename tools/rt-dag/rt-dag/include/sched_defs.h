#ifndef SCHED_DEFS_H_
#define SCHED_DEFS_H_
#include <unistd.h>

// sched_deadline syscall


// to avoid redefinition warning when compiling in __aarch64__
#ifndef __NR_sched_setattr 

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

#ifdef __aarch64__
#define __NR_sched_setattr      274
#define __NR_sched_getattr      275
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

#endif //__NR_sched_setattr

#endif // SCHED_DEFS_H_
