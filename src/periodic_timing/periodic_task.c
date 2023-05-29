
#include "periodic_task.h"

static void inc_period(struct period_info *pinfo, long delta_ns) 
{
	// add microseconds to timespecs nanosecond counter
	pinfo->next_period.tv_nsec += delta_ns;

	while (pinfo->next_period.tv_nsec >= 1000000000) {
		/* timespec nsec overflow */
		pinfo->next_period.tv_sec++;
		pinfo->next_period.tv_nsec -= 1000000000;
	}
}

void pinfo_init(struct period_info *pinfo, long period_ns)
{
	pinfo->period_ns = period_ns;
	clock_gettime(CLOCK_MONOTONIC, &(pinfo->next_period));
}

void pinfo_sum_and_wait(struct period_info *pinfo, long delta_ns)
{
	inc_period(pinfo, delta_ns);

	/* for simplicity, ignoring possibilities of signal wakes */
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &pinfo->next_period, NULL);
}

void pinfo_sum_period_and_wait(struct period_info *pinfo)
{
	pinfo_sum_and_wait(pinfo, pinfo->period_ns);
}
