#ifndef  PERIODIC_TASK_H_
#define PERIODIC_TASK_H_

#include <time.h>
/*
  Definitions for periodic tasks

Source: 
    https://wiki.linuxfoundation.org/realtime/documentation/howto/applications/cyclic

Usage:

    void task1(){

    // timing definitions
    struct period_info pinfo;
    periodic_task_init(&pinfo, 1000000);
    ...
    while (1)
    {
        ... computing ... 
        // checking the task period
        wait_rest_of_period(&pinfo);
    }
    }

*/

#ifdef  __cplusplus
extern "C" {
#endif

struct period_info {
        struct timespec next_period;
        long period_ns;
};

void pinfo_init(struct period_info *pinfo, long period_ns);

void pinfo_sum_period_and_wait(struct period_info *pinfo);

void pinfo_sum_and_wait(struct period_info *pinfo, long delta_ns);

static inline struct timespec *pinfo_get_abstime(struct period_info *pinfo) { return &pinfo->next_period; }

static inline unsigned long pinfo_get_abstime_us(struct period_info *pinfo) { return (unsigned long) (pinfo->next_period.tv_sec * 1000000L + pinfo->next_period.tv_nsec / 1000); }

#ifdef  __cplusplus
}
#endif

#endif // PERIODIC_TASK_H_
