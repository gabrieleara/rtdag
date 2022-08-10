
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

#include <sys/types.h> // to create the directory
#include <sys/stat.h>
#include <unistd.h>

#include <unistd.h> // getpid

#include <log.h>
#include "input_header.h"
#include "input_yaml.h"

// due to a failed attemp to build a proper base class, the solution if to do this 
// hack to enable switching the input type
#if INPUT_TYPE == 0 
    using input_type = input_header;
    #define INPUT_TYPE_NAME "header"
#else
    using input_type = input_yaml;
    #define INPUT_TYPE_NAME "yaml"
#endif

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

void usage(){
    cout << "./rt-dag, 2022, ReTiS Laboratory, Scuola Sant'Anna, Pisa, Italy\n";
    cout << "Compilation mode: " << INPUT_TYPE_NAME << endl;
#if INPUT_TYPE == 0 
    cout << "Usage: ./rt-dag\n";
#else
    cout << "Usage: ./rt-dag <input YAML file>\n";
//    cout << "  Options:\n";
//    cout << "    -ip power ............... Specify the upper bound for power\n";
#endif
}

int main(int argc, char* argv[]) {

  signal(SIGKILL,exit_all);
  signal(SIGSEGV,exit_all);
  signal(SIGINT,exit_all); 

  // uncomment this to get a randon seed
  //unsigned seed = time(0);
  // or set manually a constant seed to repeat the same sequence
  unsigned seed = 123456;
  cout << "SEED: " << seed << endl;  

  // this input format does not have an input file format.
  // INPUT_TYPE != 0  means this is not the input_header mode, which does not have input files
  string in_fname="";
#if INPUT_TYPE != 0 
  if (argc != 2){
      usage();
      exit(EXIT_FAILURE);
  }
  in_fname = argv[1];
#else
  if (argc != 1){
      usage();
      exit(EXIT_FAILURE);
  }
#endif  
  // read the dag configuration from the selected type of input
  std::unique_ptr< input_type > inputs = (std::unique_ptr< input_type >) new input_type(in_fname.c_str());
  // build the TaskSet class of data from dag.h
  TaskSet task_set(inputs);
  task_set.print();

  // create the directory where execution time are saved
  struct stat st = {0};
  if (stat(task_set.get_dagset_name(), &st) == -1) {
    mkdir(task_set.get_dagset_name(), 0700);
  }

  // pass pid_list such that tasks can be killed with CTRL+C
  task_set.launch_tasks(&pid_list,seed);

  LOG(INFO,"[main] all tasks were finished ...\n");

  return 0;
}
