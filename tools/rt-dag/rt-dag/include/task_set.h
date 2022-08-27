/**
 * @author Alexandre Amory, ReTiS Lab, Scuola Sant'Anna, Pisa, Italy.
 * @brief It also hides resources managment details, like mem alloc, etc. required to create trheads/process. Like in RAII style.
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
#include <pthread.h>

#include "input_wrapper.h"

using namespace std;

#define BUFFER_LINES 1

// choose the appropriate communication method based on the task implementation
#if TASK_IMPL == 0 
    // thread-based task implementation
    using cbuffer = circular_buffer <shared_mem_type,BUFFER_LINES>;
    using dag_deadline_type = circular_buffer <unsigned long,1>;
#else
    // process-based task implementation
    using cbuffer = circular_shm <shared_mem_type,BUFFER_LINES>;
    using dag_deadline_type = circular_shm <unsigned long,1>;
#endif

using ptr_cbuffer = std::shared_ptr< cbuffer >;
using vet_cbuffer = std::vector< ptr_cbuffer >;

typedef struct {
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
    pthread_barrier_t *p_bar;
    dag_deadline_type *p_dag_start_time;
    unsigned long *dag_resp_times;
} task_type;


class TaskSet{
public:
    vector< task_type > tasks;
    std::unique_ptr< input_wrapper > input;

    TaskSet(std::unique_ptr< input_wrapper > &in_data): input(move(in_data)){
        pid_list = nullptr;
        pthread_barrier_t *p_bar = (pthread_barrier_t *) malloc(sizeof(pthread_barrier_t));
        if (p_bar == NULL) {
          std::cerr << "Could not allocate pthread_barrier_t" << std::endl;
          exit(1);
        }
        int rv = pthread_barrier_init(p_bar, NULL, input->get_n_tasks());
        if (rv != 0) {
          fprintf(stderr, "barrier_init() failed: %s!\n", strerror(rv));
          exit(1);
        }
        // this is used only by the start and end tasks to check the end-to-end DAG deadline  
        dag_deadline_type *p_dag_start_time = new dag_deadline_type("dag_start_time");
        if (p_dag_start_time == 0) {
          std::cerr << "Could not allocate dag_deadline_type" << std::endl;
          exit(1);
        }
        unsigned long *dag_resp_times = new unsigned long[input->get_repetitions()];
        if (dag_resp_times == 0) {
            std::cerr << "Could not allocate dag_resp_times array with " << input->get_repetitions() << " elems!" << std::endl;
            exit(1);
        }
        tasks.resize(input->get_n_tasks());
        for(unsigned int i=0; i < input->get_n_tasks(); ++i){
            tasks[i].name = input->get_tasks_name(i);
            tasks[i].wcet = input->get_tasks_wcet(i);
            tasks[i].deadline = input->get_tasks_rel_deadline(i);
            tasks[i].affinity = input->get_tasks_affinity(i);
            // create the edges/queues w unique names
            for(unsigned int c = 0; c < input->get_n_tasks(); ++c){
                if (input->get_adjacency_matrix(i,c)!=0){
                    // TODO: the edges are now implementing 1:1 communication, 
                    // but it would be possible to have multiple readers
                    ptr_edge new_edge(new edge_type);
                    snprintf(new_edge->name, 32, "n%u_n%u", i,c);
                    new_edge->buff = (std::unique_ptr< cbuffer >) new cbuffer(new_edge->name);
                    // this message size includes the string terminator, thus, threre is no +1 here
                    new_edge->size = input->get_adjacency_matrix(i,c);
                    tasks[i].out_buffers.push_back(new_edge);
                    tasks[c].in_buffers.push_back(new_edge);
                }
            }
            tasks[i].p_bar = p_bar;
            tasks[i].p_dag_start_time = p_dag_start_time;
            // only the DAG sink should use this, but you never know...
            tasks[i].dag_resp_times = dag_resp_times;
        }
        int fd = open("/proc/sys/kernel/sched_rt_runtime_us", O_WRONLY);
        if (fd == -1) {
          perror("open() failed!");
          exit(1);
        }
        rv = write(fd, "-1\n", 3);
        if (rv < 0) {
          perror("write() failed!");
          exit(1);
        }
        close(fd);
    }

    // using shared_ptr ... no need to deallocated
    ~TaskSet(){    }

    void print() const{
        unsigned i,c;
        for(i=0;i<input->get_n_tasks();++i){
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
static void task_creator(unsigned seed, const char * dag_name, const task_type& task, const unsigned repetitions, const unsigned long dag_deadline_us, const unsigned long period_us=0){
  unsigned iter=0;
  unsigned i;
  char task_name[32];
  strcpy(task_name, task.name.c_str());
  assert((period_us != 0 && period_us>task.wcet) || period_us == 0);
  // 'seed' passed in case one needs to add some randomization in the execution time
  (void) seed;

  // sched_deadline does not support tasks shorter than 1024 ns
  if (task.wcet < 1024){
    fprintf(stderr,"ERROR: sched_deadline does not support tasks shorter than 1024 ns.\n");
    exit(1);
  }

  // set task affinity
  LOG(DEBUG,"task %s: affinity %d\n", task_name, task.affinity);
  pin_to_core(task.affinity);

  unsigned long now_long, duration;
  unsigned long task_start_time;
  string exec_time_fname;

#ifdef NDEBUG
  // file to save the task execution time in debug mode
  ofstream exec_time_f;
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
#endif // NDEBUG

  // local copy of the incomming data. this copy is not required since it is shared var,
  // but it is enforced to comply with Amalthea model 
  shared_mem_type message;

  // set the SCHED_DEADLINE policy for this task, using task.wcet as runtime and task.deadline as both deadline and period
  LOG(DEBUG,"task %s: sched wcet %lu, dline %lu\n", task_name, task.wcet, task.deadline);
  set_sched_deadline(task.wcet, task.deadline, task.deadline);

  // period definitions - used only by the starting task
  struct period_info pinfo;
  if (task.in_buffers.size() == 0) {
      pinfo_init(&pinfo, period_us * 1000);
      LOG(DEBUG, "pinfo.next_period: %ld %ld\n", pinfo.next_period.tv_sec, pinfo.next_period.tv_nsec);
  }

#if TASK_IMPL == 0 
  // wait for all threads in the DAG to have been started up to this point
  LOG(DEBUG, "barrier_wait()ing on: %p for task %s\n", (void*)task.p_bar, task.name.c_str());
  int rv = pthread_barrier_wait(task.p_bar);
  (void) rv;
  LOG(DEBUG, "barrier_wait() returned: %d\n", rv);
#endif

  if (task.in_buffers.size() == 0){
      // 1st DAG task waits 100ms to make sure its in-kernel CBS deadline is aligned with the abs deadline in pinfo
      LOG(DEBUG, "waiting 100ms\n");
      pinfo_sum_and_wait(&pinfo, 100*1000*1000);
      LOG(DEBUG, "woken up: pinfo.next_period: %ld %ld\n", pinfo.next_period.tv_sec, pinfo.next_period.tv_nsec);
  }

  while(iter < repetitions){
    // check the end-to-end DAG deadline.
    // create a shared variable with the start time of the dag such that the final task can check the dag deadline.
    // this variable is set by the starting task and read by the final task.
    // if this is the starting task, i.e. a task with no input queues, get the time the dag started.
    if (task.in_buffers.size() == 0){
      // on 1st iteration, this is the time pinfo_init() was called;
      // on others, it is the exact (theoretical) wake-up time of the DAG;
      // this is what we compute response times (and possible deadline misses) against
      // (NOT the first time the task is scheduled by the OS, that can be a much later time)
      now_long = pinfo_get_abstime_us(&pinfo);
      task.p_dag_start_time->push(now_long);
      LOG(DEBUG,"task %s (%u): dag start time %lu\n", task_name, iter, now_long);
      LOG(DEBUG, "pinfo.next_period: %ld %ld\n", pinfo.next_period.tv_sec, pinfo.next_period.tv_nsec);
    }

    // wait all incomming messages
    LOG(INFO,"task %s (%u): waiting msgs\n", task_name, iter);
    for(i=0;i<task.in_buffers.size();++i){
        LOG(INFO,"task %s (%u), waiting buffer %s(%d)\n", task_name, iter, task.in_buffers[i]->name, task.in_buffers[i]->size);
        // it blocks until the data is produced
        task.in_buffers[i]->buff->pop(message);
        LOG(INFO,"task %s (%u), buffer %s(%d): got message: '%s'\n", task_name, iter, task.in_buffers[i]->name, message.size(), message.get());
    }

    unsigned long wcet = ((float)task.wcet)*0.95f;
    LOG(INFO,"task %s (%u): running the processing step\n", task_name, iter);
    // the task execution time starts to count only after all incomming msgs were received
    task_start_time = (unsigned long) micros(); 
    // runs busy waiting to mimic some actual processing.
    // using sleep or wait wont achieve the same result, for instance, in power consumption
    Count_Time(wcet);

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
    LOG(INFO,"task %s (%u): task duration %lu us\n", task_name, iter, duration);

#ifdef NDEBUG
    // write the task execution time into its log file
    exec_time_f << duration << endl;
    if (duration > task.deadline){
        printf("ERROR: task %s (%u): task duration %lu > deadline %lu!\n", task_name, iter, duration, task.deadline);
        //TODO: stop or continue ?
    }
#endif // NDEBUG

    // if this is the final task, i.e. a task with no output queues, check the overall dag execution time
    if (task.out_buffers.size() == 0){
        unsigned long last_dag_start;
        task.p_dag_start_time->pop(last_dag_start);
        duration = now_long - last_dag_start;
        LOG(INFO, "task %s (%u): dag duration %lu us = %lu ms = %lu s\n\n", task_name, iter, duration, US_TO_MSEC(duration), US_TO_SEC(duration));
        task.dag_resp_times[iter] = duration;
        if (duration > dag_deadline_us){
            // we do expect a few deadline misses, despite all precautions, we'll find them in the output file
            LOG(DEBUG, "ERROR: dag deadline violation detected in iteration %u. duration %ld us\n", iter, duration);
        }
    }

    // only the start task waits for the period
    // OBS: not sure if this part of the code is in its correct/most precise position
    if (task.in_buffers.size() == 0){
        LOG(DEBUG, "waiting till pinfo.next_period: %ld %ld\n", pinfo.next_period.tv_sec, pinfo.next_period.tv_nsec);
        pinfo_sum_period_and_wait(&pinfo);
        LOG(DEBUG, "woken up on new instance, pinfo.next_period: %ld %ld\n", pinfo.next_period.tv_sec, pinfo.next_period.tv_nsec);
    }

    ++iter;
  }

#ifdef NDEBUG
  exec_time_f.close();
#endif // NDEBUG

  if (task.out_buffers.size() == 0){
      // file to save the dag execution time, created only by the end task
      ofstream dag_exec_time_f;
      exec_time_fname = dag_name;  
      exec_time_fname += "/";
      exec_time_fname += dag_name;
      exec_time_fname += ".log";
      dag_exec_time_f.open(exec_time_fname, std::ios_base::app);
      if (! dag_exec_time_f.is_open()){
          fprintf(stderr, "ERROR: execution time '%s' file not created\n", exec_time_fname.c_str());
          exit(1);
      }
      // the 1st line is the task relative deadline. all the following lines are actual execution times
      // TODO tiny hidden bug here that happens only when the experiment is run multiple times.
      // it must 1st detect if the file exist. If so, then the deadline is not written again in the file.
      dag_exec_time_f << dag_deadline_us << endl;
      for (unsigned int i = 0; i < repetitions; i++)
          dag_exec_time_f << task.dag_resp_times[i] << endl;
      dag_exec_time_f.close();
  }
}

    void thread_launcher(unsigned seed){
        vector<std::thread> threads;
        unsigned long thread_id;
        threads.push_back(thread(task_creator,seed, input->get_dagset_name(), tasks[0], input->get_repetitions(), input->get_deadline(), input->get_period()));
        thread_id = std::hash<std::thread::id>{}(threads.back().get_id());
        pid_list->push_back(thread_id);
        LOG(INFO,"[main] pid %d task 0\n", getpid());
        for (unsigned i = 1; i < input->get_n_tasks(); i++) {
            threads.push_back(std::thread(task_creator, seed, input->get_dagset_name(), tasks[i], input->get_repetitions(), input->get_deadline(), 0));
            thread_id = std::hash<std::thread::id>{}(threads.back().get_id());
            pid_list->push_back(thread_id);
            LOG(INFO,"[main] pid %d task %d\n", getpid(), i);
        }
        for (auto &th : threads) {
            th.join();
        }        
    }

#if TASK_IMPL != 0 
    void process_launcher(unsigned seed){
        this->spawn_proc(tasks[0],seed,input->get_period());
        for(unsigned i=1;i<input->get_n_tasks();++i){
            this->spawn_proc(tasks[i],seed,0);
        }
        // join processes

        // a simpler way to wait the tasks ...
        // while ((wpid = wait(&status)) > 0); // this way, the father waits for all the child processes 
        // a beter way to make sure all the tasks were closed
        // https://stackoverflow.com/questions/8679226/does-a-kill-signal-exit-a-process-immediately
        assert(pid_list->size()==input->get_n_tasks());
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
            perror("ERROR Fork Failed");
            exit(-1);
        }
        if (pid == 0){
            printf("Task %s pid %d forked\n",task.name.c_str(),getpid());
            task_creator(seed, input->get_dagset_name(), task, input->get_repetitions(), input->get_deadline(), period);
            exit(0);
        }else{
            pid_list->push_back(pid);
            LOG(INFO,"parent %d forked task %d\n", getppid(), pid);
        }
    }
#endif

    static void set_sched_deadline(unsigned long runtime, unsigned long deadline, unsigned long period ){
        struct sched_attr sa ;
        if (sched_getattr(0, &sa, sizeof(sa), 0) < 0) {
            perror("ERROR sched_getattr()");
            exit(1);
        }
        sa.sched_policy   = SCHED_DEADLINE;
        // time in microseconds
        sa.sched_runtime  = runtime;
        sa.sched_deadline = deadline;
        sa.sched_period   = period;
        if (sched_setattr( 0, &sa, 0) < 0)
        {
            perror("ERROR sched_setattr()");
            printf("ERROR: make sure you run rt-dag with 'sudo' and also 'echo -1 | sudo tee /proc/sys/kernel/sched_rt_runtime_us' is executed before running rt-dag\n");
            exit(1);
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
