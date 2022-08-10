/**
 * @author Alexandre Amory, ReTiS Lab, Scuola Sant'Anna, Pisa, Italy.
 * @brief It transfers data from dag.h into a more managable data structure.
 * It also hides resources managment details, like mem alloc, etc. Like in RAII style.
 * @version 0.1
 * @date 2022-07-06
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef TASK_SET_H_
#define TASK_SET_H_

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>  // find
#include <sys/wait.h> // waitpid
#include <sched.h>    // sched_setaffinity
// to set the sched_deadline parameters
#include <linux/kernel.h>
#include <linux/unistd.h>
// #include <time.h>
#include <linux/types.h>

#include <periodic_task.h>
#include <time_aux.h>

#include "shared_mem_type.h"
#include "circular_buffer.h"
#include "circular_shm.h"
#include "sched_defs.h"

using namespace std;

#define BUFFER_LINES 1

// choose the appropriate communication method based on the task implementation
#if TASK_IMPL == 0 
    // thread-based task implementation
    using cbuffer = circular_buffer <shared_mem_type,BUFFER_LINES>;
#else
    // process-based task implementation
    using cbuffer = circular_shm <shared_mem_type,BUFFER_LINES>;
#endif

using ptr_cbuffer = std::shared_ptr< cbuffer >;
using vet_cbuffer = std::vector< ptr_cbuffer >;

// POSIX shared memory are used both for thread/process task implementation
using dag_deadline_type = circular_shm <unsigned long,1>;


typedef struct {
    // the only reason this is pointer is that, when using circular_shm, it requries to pass the shared mem name
    std::unique_ptr< cbuffer > buff;
    unsigned size; // in bytes
    char name[32];
} edge_type;

using ptr_edge = std::shared_ptr< edge_type >;

typedef struct {
    string name;
    unsigned affinity; // which core the task is mapped
    unsigned long wcet; // in us 
    unsigned long deadline; // in us
    vector< ptr_edge > in_buffers;
    vector< ptr_edge > out_buffers;
} task_type;


class TaskSet{
public:
    vector< task_type > tasks;
    std::unique_ptr< input_type > input;

    TaskSet(std::unique_ptr< input_type > &in_data): input(move(in_data)){
        unsigned i,c;
        pid_list = nullptr;
        tasks.resize(N_TASKS);
        for(i=0;i<N_TASKS;++i){
            tasks[i].name = tasks_name[i];
            tasks[i].wcet = tasks_wcet[i];
            tasks[i].deadline = tasks_rel_deadline[i];
            tasks[i].affinity = task_affinity[i];
            // create the edges/queues w unique names
            for(c=0;c<N_TASKS;++c){
                if (adjacency_matrix[i][c]!=0){
                    // TODO: the edges are now implementing 1:1 communication, 
                    // but it would be possible to have multiple readers
                    ptr_edge new_edge(new edge_type);
                    snprintf(new_edge->name, 32, "n%u_n%u", i,c);
                    new_edge->buff = (std::unique_ptr< cbuffer >) new cbuffer(new_edge->name);
                    // this message size includes the string terminator, thus, threre is no +1 here
                    new_edge->size = adjacency_matrix[i][c];
                    tasks[i].out_buffers.push_back(new_edge);
                    tasks[c].in_buffers.push_back(new_edge);
                }
            }
        }
    }

    // using shared_ptr ... no need to deallocated
    ~TaskSet(){    }

    void print() const{
        unsigned i,c;
        for(i=0;i<N_TASKS;++i){
            cout << tasks[i].name << ", wcet: " << tasks[i].wcet  << ", deadline: " << tasks[i].deadline << ", affinity: " << tasks[i].affinity << endl;
            cout << " ins: ";
            for(c=0;c<tasks[i].in_buffers.size();++c)
                cout << tasks[i].in_buffers[c]->name << "(" << tasks[i].in_buffers[c]->size << "), ";
            cout << endl;
            cout << " outs: ";
            for(c=0;c<tasks[i].out_buffers.size();++c)
                cout << tasks[i].out_buffers[c]->name << "(" << tasks[i].out_buffers[c]->size << "), ";
            cout << endl;
        }    
  }

  void launch_tasks(vector<int> *task_id, unsigned seed){
    pid_list = task_id;
    #if TASK_IMPL == 0 
        thread_launcher(seed);
    #else
        process_launcher(seed);
    #endif
  }

  const char *get_dagset_name() const {return input->get_dagset_name();}
private:
    
    
    // used only in process mode to keep the pid # of each task, enabling to kill the tasks CTRL+C
    vector<int> *pid_list;

// This is the main method that actually implements the task behaviour. It reads its inputs
// execute some dummy processing in busi-wait, and sends its outputs to the next tasks.
// 'period_ns' argument is only used when the task is periodic, which is tipically only the first tasks of the DAG
static void task_creator(unsigned seed, const char * dag_name, const task_type& task, const unsigned long period_ns=0){
  unsigned iter=0;
  unsigned i;
  unsigned long execution_time;
  float rnd_val;
  char task_name[32];
  strcpy(task_name, task.name.c_str());
  assert((period_ns != 0 && period_ns>task.wcet) || period_ns == 0);

  // randomize the start time of each task
//   std::mt19937_64 eng{std::random_device{}()};  // or seed however you want
//   std::uniform_int_distribution<> dist{10, 100};
//   std::this_thread::sleep_for(std::chrono::milliseconds{dist(eng)});
    
//   std::hash<std::thread::id> myHashObject{};
//   uint32_t threadID   __attribute__((unused)) = myHashObject(std::this_thread::get_id());
//   LOG(INFO,"task %s created: pid = %u, ppid = %d\n", task_name, threadID, getppid());

  // set task affinity
  pin_to_core(task.affinity);

  // set the SCHED_DEADLINE policy for this task, using task.wcet as runtime and task.deadline as both deadline and period
  set_sched_deadline(task.wcet, task.deadline, task.deadline);

  // this is used only by the start and end tasks to check the end-to-end DAG deadline  
  dag_deadline_type dag_start_time("dag_start_time");

  // each task has its own rnd engine to limit blocking for shared resources.
  // the thread seed is built by summin up the main seed + a hash of the task name, which is unique
  seed+= std::hash<std::string>{}(task_name);
  std::mt19937_64 engine(static_cast<uint64_t> (seed));
  // each task might use different distributions
  std::uniform_real_distribution<double> zeroToOne(0.0, 1.0);

  // period definitions - used only by the starting task
  struct period_info pinfo;
  pinfo.period_ns = period_ns;
  periodic_task_init(&pinfo);

  unsigned long now_long, duration;
  unsigned long task_start_time;

  // file to save the task execution time. the dummy tasks are not included
  string exec_time_fname;
  ofstream exec_time_f;
  if (task.deadline > 0){
    exec_time_fname = dag_name;  
    exec_time_fname += "/";
    exec_time_fname += task.name;
    exec_time_fname += ".log";
    exec_time_f.open(exec_time_fname, std::ios_base::app);
    if (! exec_time_f.is_open()){
        printf("ERROR: execution time '%s' file not created\n", exec_time_fname.c_str());
        exit(1);
    }
    // the 1st line is the task relative deadline. all the following lines are actual execution times
    exec_time_f << task.deadline << endl;
  }

  // file to save the dag execution time, created only by the end task
  ofstream dag_exec_time_f;
  if (task.out_buffers.size() == 0){
    exec_time_fname = dag_name;  
    exec_time_fname += "/";
    exec_time_fname += dag_name;
    exec_time_fname += ".log";
    dag_exec_time_f.open(exec_time_fname, std::ios_base::app);
    if (! dag_exec_time_f.is_open()){
        printf("ERROR: execution time '%s' file not created\n", exec_time_fname.c_str());
        exit(1);
    }
    // the 1st line is the task relative deadline. all the following lines are actual execution times
    dag_exec_time_f << DAG_DEADLINE << endl;
  }

  // local copy of the incomming data. this copy is not required since it is shared var,
  // but it is enforced to comply with Amalthea model 
  shared_mem_type message;
  while(iter < REPETITIONS){
    // check the end-to-end DAG deadline.
    // create a shared variable with the start time of the dag such that the final task can check the dag deadline.
    // this variable is set by the starting task and read by the final task.
    // if this is the starting task, i.e. a task with no input queues, get the time the dag started.
    if (task.in_buffers.size() == 0){
      now_long = (unsigned long) micros(); 
      dag_start_time.push(now_long);
      LOG(DEBUG,"task %s (%u): dag start time %lu\n", task_name, iter, now_long);
    }

    // wait all incomming messages
    LOG(INFO,"task %s (%u): waiting msgs\n", task_name, iter);
    for(i=0;i<task.in_buffers.size();++i){
        LOG(INFO,"task %s (%u), waiting buffer %s(%d)\n", task_name, iter, task.in_buffers[i]->name, task.in_buffers[i]->size);
        // it blocks until the data is produced
        task.in_buffers[i]->buff->pop(message);
        LOG(INFO,"task %s (%u), buffer %s(%d): got message: '%s'\n", task_name, iter, task.in_buffers[i]->name, message.size(), message.get());
    }

    // randomize the actual execution time of each iteration
    rnd_val = zeroToOne(engine);
    // 0.98 is used to reduce the actual execution by 2% such that, hopefully, the wcet is not always violated 
    // TODO: there must be a better way to do this
    float wcet = ((float)task.wcet)*0.98f;
    execution_time = 4.0f/5.0f*wcet + 1.0f/5.0f*rnd_val;
    LOG(INFO,"task %s (%u): running the processing step\n", task_name, iter);
    task_start_time = (unsigned long) micros(); 
    // runs busy waiting to mimic some actual processing.
    // using sleep or wait wont achieve the same result, for instance, in power consumption
    Count_Time(execution_time);
    //usleep(execution_time);
    //Count_Time((int)wcet)
    // usleep((int)wcet);

    // send data to the next tasks. in release mode, the time to send msgs (when no blocking) is about 50 us
    LOG(INFO,"task %s (%u): sending msgs!\n", task_name,iter);
    for(i=0;i<task.out_buffers.size();++i){
        message.set(task_name,iter,task.out_buffers[i]->size);
        //assert(message.size() < task.out_buffers[i]->size);
        task.out_buffers[i]->buff->push(message);
        LOG(INFO,"task %s (%u): buffer %s, size %u, sent message: '%s'\n",task_name, iter, task.out_buffers[i]->name, message.size(), message.get());
    }
    LOG(INFO,"task %s (%u): all msgs sent!\n", task_name, iter);

    now_long = micros();
    duration = now_long - task_start_time;
    printf("task %s (%u): task duration %lu us\n", task_name, iter, duration);
    if (task.deadline > 0){
        exec_time_f << duration << endl;
    }
    // check the duration of the tasks if this is in conformance w their wcet.
    // tasks with wcet == 0 or deadline==0, like initial and final tasks, are not checked 
    if (duration > task.wcet && task.wcet > 0){
        printf("task %s (%u): task duration %lu > wcet %lu!\n", task_name, iter, duration, task.wcet);
    }
    if (duration > task.deadline && task.deadline > 0){
        printf("ERROR: task %s (%u): task duration %lu > deadline %lu!\n", task_name, iter, duration, task.deadline);
        //TODO: stop or continue ?
    }

    // only the start task waits for the period
    if (task.in_buffers.size() == 0){
        wait_rest_of_period(&pinfo);
    }

    // if this is the final task, i.e. a task with no output queues, check the overall dag execution time
    if (task.out_buffers.size() == 0){
        unsigned long last_dag_start;
        dag_start_time.pop(last_dag_start);
        duration = now_long - last_dag_start;
        LOG(INFO,"task %s (%u): dag duration %lu - %lu = %lu us = %lu ms = %lu s\n\n", task_name, iter, now_long, last_dag_start, duration, US_TO_MSEC(duration), US_TO_SEC(duration));
        printf("task %s (%u): dag  duration %lu us\n\n", task_name, iter,  duration);
        dag_exec_time_f << duration << endl;
        if (duration > DAG_DEADLINE){
            printf("ERROR: dag deadline violation detected in iteration %u. duration %ld us\n", iter, duration);
            assert(duration <= DAG_DEADLINE);
        }
    }
    ++iter;
  }
  if (task.deadline > 0){
    exec_time_f.close();
  }
  if (task.out_buffers.size() == 0){
    dag_exec_time_f.close();
  }
}

    void thread_launcher(unsigned seed){
        vector<std::thread> threads;
        unsigned long thread_id;
        threads.push_back(thread(task_creator,seed, input->get_dagset_name(), tasks[0], input->get_period()));
        thread_id = std::hash<std::thread::id>{}(threads.back().get_id());
        pid_list->push_back(thread_id);
        LOG(INFO,"[main] pid %d task 0\n", getpid());
        for (unsigned i = 1; i < N_TASKS; i++) {
            threads.push_back(std::thread(task_creator, seed, input->get_dagset_name(), tasks[i], 0));
            thread_id = std::hash<std::thread::id>{}(threads.back().get_id());
            pid_list->push_back(thread_id);
            LOG(INFO,"[main] pid %d task %d\n", getpid(), i);
        }
        for (auto &th : threads) {
            th.join();
        }        
    }

    void process_launcher(unsigned seed){
        this->spawn_proc(tasks[0],seed,DAG_PERIOD);
        for(unsigned i=1;i<N_TASKS;++i){
            this->spawn_proc(tasks[i],seed,0);
        }
        // join processes

        // a simpler way to wait the tasks ...
        // while ((wpid = wait(&status)) > 0); // this way, the father waits for all the child processes 
        // a beter way to make sure all the tasks were closed
        // https://stackoverflow.com/questions/8679226/does-a-kill-signal-exit-a-process-immediately
        assert(pid_list->size()==N_TASKS);
        // this one is deleted 
        vector<int> local_task_id(*pid_list);
        while( local_task_id.size() != 0 ){
            int pid = (int)waitpid(-1, NULL, WNOHANG);
            if( pid > 0 ){
                // recover the task index w this PID to delete it from the list
                auto task_it = find(local_task_id.begin(),local_task_id.end(), pid);
                if (task_it == local_task_id.end()){
                    printf("ERROR: waitpid returned a invalid PID ?!?!\n");
                    break;
                }else{
                    local_task_id.erase(task_it);
                }
                // this find is done only to get the name of the killed task 
                task_it = find(pid_list->begin(),pid_list->end(), pid);
                unsigned task_idx = task_it - pid_list->begin();                    
                printf("Task %s pid %d killed\n", tasks[task_idx].name.c_str(), pid);
            }else if( pid == 0 ){
                sleep(1);
            }else{
                printf("WARNING: something went wrong in the task finishing procedure!\n");
                break;
            }
        }
    }

    // does fork checking and save the PID
    void spawn_proc(const task_type& task, const unsigned seed, const unsigned period){
        int pid = (int)fork();
        if (pid < 0){
            fprintf(stderr, "Fork Failed");
            exit(-1);
        }
        if (pid == 0){
            printf("Task %s pid %d forked\n",task.name.c_str(),getpid());
            task_creator(seed, input->get_dagset_name(), task, period);
            exit(0);
        }else{
            pid_list->push_back(pid);
            LOG(INFO,"parent %d forked task %d\n", getppid(), pid);
        }
    }

    static void set_sched_deadline(unsigned long runtime, unsigned long deadline, unsigned long period ){
        struct sched_attr sa ;
        if (sched_getattr(0, &sa, sizeof(sa), 0) < 0) {
            cerr << "Error sched_getattr()" << endl;
            exit(1);
        }
        sa.sched_policy   = SCHED_DEADLINE;
        // time in microseconds
        sa.sched_runtime  = runtime;
        sa.sched_deadline = deadline;
        sa.sched_period   = period;
        if (sched_setattr( 0, &sa, 0) < 0)
        {
            cerr << "Error sched_setattr()" << endl;
        }
    }

    static void pin_to_core(const unsigned cpu){
        #if TASK_IMPL == 0 
            pin_thread(cpu);
        #else
            pin_process(cpu);
        #endif
    }

    // https://github.com/rigtorp/SPSCQueue/blob/master/src/SPSCQueueBenchmark.cpp
    static void pin_thread(const unsigned cpu) {
        // WARNINIG: there are situations that the max number of threads can be wrong
        // https://stackoverflow.com/questions/57298045/maximum-number-of-threads-in-c
        assert(cpu < std::thread::hardware_concurrency());
        cpu_set_t cpuset;
        int ret;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu, &cpuset);
        ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
        assert(ret==0);
    }

    static void pin_process(const unsigned cpu){
        cpu_set_t  mask;
        int ret;
        CPU_ZERO(&mask);
        CPU_SET(cpu, &mask);
        ret = sched_setaffinity(getpid(), sizeof(mask), &mask);
        assert(ret==0);
    }

};

#endif // TASK_SET_H_
