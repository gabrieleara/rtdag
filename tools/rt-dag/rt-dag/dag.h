#ifndef DAG_H_
#define DAG_H_

// dagset_name and tasks_name are used to save the execution times into files organized as: 
// ./dagset_name/tasks_name.log
const char * dagset_name = "rnd00";
#define N_TASKS 6
#define N_EDGES 6
#define MAX_OUT_EDGES_PER_TASK 2
#define MAX_IN_EDGES_PER_TASK  2
#define MAX_MSG_LEN 53
#define REPETITIONS 5 // the number of iterations of the complete DAG
#define DAG_PERIOD 1'000'000 // in us
#define DAG_DEADLINE DAG_PERIOD // usually is the same as dag period, but not necessarly
const char * tasks_name[N_TASKS] = {"input","n0","n1","n2","n3","output",};
const unsigned tasks_wcet[N_TASKS] = {0,50'000,500'000,200'000,100'000,0}; // in us. 
// The relative deadline of each task.
// make sure that the sum of the longest path must be <= DAG_DEADLINE since
// this consistency is not done here !!!
const unsigned tasks_rel_deadline[N_TASKS] = {0,100'000,800'000,800'000,100'000,0}; // in us
// pin threads/processes onto the specified cores
const unsigned task_affinity[N_TASKS] = {0,1,2,3,1,0};
// values != 0 means there is a link from task l (line) to task c(column)
// amount of bytes sent by each edge
const unsigned adjacency_matrix[N_TASKS][N_TASKS] = {
{0,2,0,0,0,0,},
{0,0,31,51,0,0,},
{0,0,0,0,33,0,},
{0,0,0,0,53,0,},
{0,0,0,0,0,2,},
{0,0,0,0,0,0,},
};

// TODO: set task scheduling attributes w struct sched_attr

#endif // DAG_H_
