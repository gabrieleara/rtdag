#ifndef DAG_H_
#define DAG_H_

// global defs
const char *dagset_name = "minimal_header";
#define N_TASKS 4
#define N_EDGES 4
#define N_CPUS 8
#define MAX_OUT_EDGES_PER_TASK 2
#define MAX_IN_EDGES_PER_TASK 2
#define MAX_MSG_LEN 64
#define REPETITIONS 50     // the number of iterations of the complete DAG
#define DAG_PERIOD 300'000 // in us
#define DAG_DEADLINE                                                           \
    DAG_PERIOD // usually is the same as dag period, but not necessarly
#define HYPERPERIOD                                                            \
    300'000 // in us . relevant only when it runs w multidags scenarios, i.e.,
            // multi rt-dag processes concurrently
const char *tasks_name[N_TASKS] = {"n0", "n1", "n2", "n3"};
// The actual task computation workload in 'ticks'. a tick is equivalent to a
// simple logic operation. Note that tick is not time ! it's a kind of workload
// unit
const unsigned tasks_wcet[N_TASKS] = {50'000, 50'000, 50'000,
                                      50'000}; // in ticks.
// The relative deadline of each task.
// make sure that the sum of the longest path must be <= DAG_DEADLINE since
// this consistency is not done here !!!
const unsigned tasks_rel_deadline[N_TASKS] = {100'000, 100'000, 100'000,
                                              100'000}; // in us
// pin threads/processes onto the specified cores
// the values are the cpu ids. mask is currently not supported
const unsigned task_affinity[N_TASKS] = {1, 2, 2, 4};
// values != 0 means there is a link from task l (line) to task c(column)
// amount of bytes sent byeach edge
const unsigned adjacency_matrix[N_TASKS][N_TASKS] = {
    {0, 30, 50, 0},
    {0, 0, 0, 32},
    {0, 0, 0, 52},
    {0, 0, 0, 0},
};
#endif // DAG_H_
