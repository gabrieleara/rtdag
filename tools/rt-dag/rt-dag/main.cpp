
/*
 * Description: This application tests how to build a taskset described as a DAG using threads and a thread-safe queue for communication.
 * 
 * Example DAG:
 *     n1
 *    /   \
 * n0      n3
 *    \   /
 *     n2 
 * 
 * n3 can only be unlocked when all its antecessors n1 and n2 finished.
 * 
 *
 * Assumptions:
 *  - Each task is implemented w a thread;
 *  - Each edge is implemented with a thread-safe queue w size limite and blocking push. Syncronous blocking communication mode.
 *  - The DAG period is lower than the DAG WCET. This way, each shared variable it's garanteed that the next element will be received only after the current one is consumed
 *  - The 'task duration' accounts for the task execution plus msgs sent. It does not account waiting time for incomming data
 * 
 * Author: 
 *  Alexandre Amory (June 2022), ReTiS Lab, Scuola Sant'Anna, Pisa, Italy.
 * 
 * Compilation: 
 *  $> g++  main.cpp ../../../common/src/periodic_task.c ../../../common/src/time_aux.c -I ../../../common/include/ -I blocking_queue -g3 -o dag-launcher-thread -lrt -lpthread
 *  
 * Usage Example: 
 *  $> ./dag-launcher-thread
 */

#include <iostream>           // std::cout
#include <thread>             // std::thread
#include <vector>
#include <random>
#include <string>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <unistd.h> // getpid

#include <log.h>
#include <periodic_task.h>
#include <time_aux.h>

// this tells to use thread-safe circular buffer 
// the dag definition is here
#include "task_set.h"

using namespace std;

////////////////////////////
// globals used by the tasks
////////////////////////////
// check the end-to-end DAG deadline
// the start task writes and the final task reads from it to 
// unsigned long dag_start_time; 
vector<int> pid_list;

void exit_all(int sigid){
#if TASK_IMPL == 0 
    printf("Killing all threads\n");
    // TODO: how to kill the threads without access to the thread list ?
#else
    printf("Killing all tasks\n");
    unsigned i,ret;
    for(i=0;i<pid_list.size();++i){
        ret = kill(pid_list[i],SIGKILL);
        assert(ret==0);
    }
#endif    
    printf("Exting\n");
    exit(0);
}

int main() {

  signal(SIGKILL,exit_all);
  signal(SIGSEGV,exit_all);
  signal(SIGINT,exit_all); 

  // uncomment this to get a randon seed
  //unsigned seed = time(0);
  // or set manually a constant seed to repeat the same sequence
  unsigned seed = 123456;
  cout << "SEED: " << seed << endl;  

  // build the TaskSet class of data from dag.h
  TaskSet task_set;
  task_set.print();
  // pass pid_list such that tasks can be killed with CTRL+C
  task_set.launch_tasks(&pid_list,seed);

  LOG(INFO,"[main] all tasks were finished ...\n");

  return 0;
}
