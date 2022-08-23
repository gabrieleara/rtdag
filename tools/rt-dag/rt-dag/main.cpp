
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
 *  - Each task is implemented w a thread or process. Check TASK_IMPL define;
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

#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <string>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <sys/types.h> // to create the directory
#include <sys/stat.h>

#include <unistd.h> // getpid

#include <log.h>
#include "input_wrapper.h"

// select the input type
#if INPUT_TYPE == 0 
    #include "input_header.h"
    using input_type = input_header;
    #define INPUT_TYPE_NAME "header"
#else
    #include "input_yaml.h"
    using input_type = input_yaml;
    #define INPUT_TYPE_NAME "yaml"
#endif

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

// TODO: restore the previously saved freq setup before starting rt-dag
void restore_cpu_freq(){}

// TODO: save the previous freq setup to be restored after the rt-dag is executed
void save_cpu_freq(){}


void exit_all(int sigid){
    restore_cpu_freq();
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
    cout << "Input mode: " << INPUT_TYPE_NAME << endl;
#if INPUT_TYPE == 0 
    cout << "Usage: ./rt-dag\n";
#else
    cout << "Usage: ./rt-dag <input YAML file>\n";
//    cout << "  Options:\n";
//    cout << "    -ip power ............... Specify the upper bound for power\n";
#endif
}

void set_cpu_freq(std::unique_ptr< input_wrapper > &in_data){
    //  TODO: use 'cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq' for debug
    const unsigned ncpus = in_data->get_n_cpus();
    unsigned freq=0;
    char cmd[128];
    // save the current cpu freq setup to be restored after the rt-dag execution
    save_cpu_freq();

    // TODO: read the available governor to check whether 'userspace' is listed

    // set the governor
    // OBS: if your kernel is not configured with 'userspace' governor, you can still
    // set the freq you want by using the 'performance' governor and setting the 
    // frequency in 'scaling_max_freq'
    // source: https://askubuntu.com/questions/20271/how-do-i-set-the-cpu-frequency-scaling-governor-for-all-cores-at-once
    int rv = system("sudo bash -c 'for ((i=0;i<$(nproc);i++)); do cpufreq-set -c $i -g userspace; done'");
    if (rv != 0) {
        perror("ERROR setting governor");
        printf("ERROR: make sure the OS has the 'userspace' governor enabled by running 'cat /sys/devices/system/cpu/cpu1/cpufreq/scaling_available_governors'.");
        exit(1);
    }    
    for (unsigned i=0; i< ncpus; i++){
        freq = in_data->get_cpus_freq(i);
        rv = snprintf(cmd, sizeof(cmd), "sudo cpufreq-set -c %d -f %dMhz",i, freq);
        assert((unsigned)rv < sizeof(cmd));
        rv = system(cmd);
        if (rv != 0) {
            perror("ERROR setting the CPU frequency");
            exit(1);
        }
    }
}


int main(int argc, char* argv[]) {

  signal(SIGKILL,exit_all);
  signal(SIGSEGV,exit_all);
  signal(SIGINT,exit_all); 

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

  // uncomment this to get a randon seed
  //unsigned seed = time(0);
  // or set manually a constant seed to repeat the same sequence
  unsigned seed = 123456;
  cout << "SEED: " << seed << endl;  

  // read the dag configuration from the selected type of input
  std::unique_ptr< input_wrapper > inputs = (std::unique_ptr< input_wrapper >) new input_type(in_fname.c_str());
  inputs->dump();
  // set the CPU frequencies
#ifdef SET_FREQ    
  set_cpu_freq(inputs);
#endif
  // build the TaskSet class of data from dag.h
  TaskSet task_set(inputs);
  task_set.print();

  // create the directory where execution time are saved
  struct stat st = {0};
  if (stat(task_set.get_dagset_name(), &st) == -1) {
    // permisions required in order to allow using rsync since rt-dag is run as root in the target computer
    int rv = mkdir(task_set.get_dagset_name(), 0777);
    if (rv != 0) {
        perror("ERROR creating directory");
        exit(1);
    }    
  }

  // pass pid_list such that tasks can be killed with CTRL+C
  task_set.launch_tasks(&pid_list,seed);

  LOG(INFO,"[main] all tasks were finished ...\n");
  // restored to the previous CPU frequenies
  restore_cpu_freq();

  return 0;
}
