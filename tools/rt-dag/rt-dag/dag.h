#ifndef DAG_H_
#define DAG_H_

// global defs
#define N_TASKS 4
#define N_EDGES 4
#define MAX_OUT_EDGES_PER_TASK 2
#define MAX_IN_EDGES_PER_TASK 2
#define MAX_MSG_LEN 256
#define REPETITIONS 5 // the number of iterations of the complete DAG
#define DAG_PERIOD 1'000'000 // in us 
#define DAG_DEADLINE DAG_PERIOD // usually is the same as dag period, but not necessarly
// The actual task computation time is decided randomly in runtime
const unsigned tasks_wcet[N_TASKS] = {50'000,500'000,200'000,50'000}; // in us. 
// The relative deadline of each task.
// make sure that the sum of the longest path must be <= DAG_DEADLINE since
// this consistency is not done here !!!
const unsigned tasks_rel_deadline[N_TASKS] = {100'000,800'000,800'000,100'000}; // in us
// pin threads/processes onto the specified cores
// the values are the cpu ids. mask is currently not supported
const unsigned task_affinity[N_TASKS] = {1,2,3,1};
// values != 0 means there is a link from task l (line) to task c(column)
// amount of bytes sent byeach edge
const unsigned adjacency_matrix[N_TASKS][N_TASKS] = {
    {0,30,50, 0},
    {0, 0, 0,32},
    {0, 0, 0,52},
    {0, 0, 0, 0},
};

// TODO: extend the data structure to set the frequency of the islands
// https://www.thinkwiki.org/wiki/How_to_make_use_of_Dynamic_Frequency_Scaling
// TODO: set task affinity w cpu_set_t
// TODO: set task scheduling attributes w struct sched_attr


#endif // DAG_H_
