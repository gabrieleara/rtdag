# Thread-based DAG launcher

Example application that, given a certain real-time DAG description, it launches a set of Linux threads that represents this DAG including, for instance, the DAG period and end-to-end deadline. 

# What does it do ?

Part of the realtime community uses workloads modeled as DAGs. However, it's not easy to find benchmarks and applications to evaluate the research proposals on top of actual Linux OS and hardware platforms. Sometimes the design team has traces of individual tasks, but not for the entire DAG. In those situations, this application could be used to do some preliminary integration of tasks of a DAG to check the overall system performance parameters.

This application can be used together with a *random graph generator* or *traces of an actual application* to quickly model a DAG application, with some dummy computation (i.e. busy loop) and actual communication. Then, this DAG can be used:
  - to check the OS response times under a certain DAG workload. For instance, it's possible to quickly tweak the WCET of individual tasks to see its impact on the DAG end-to-end deadline. This can be used to decide in which task is more interesting to focus for WCET optimizations;
  - to check computing/communication/IO bottlenecks: can be accomplished in combination with standard [Linux performance monitoring tools](https://www.tecmint.com/command-line-tools-to-monitor-linux-performance/); 
  - to check for memory *collision* in the cache: can be accomplished with [perf](https://www.brendangregg.com/perf.html);
  - to check whether the workload is within the power budget (TO BE DONE!);
  - to check if the DAG end-to-end deadline is respected (DONE!).

# How does it work ?

The user describes a certain real-time DAG (see [dag.h](./dag.h) for an example) which represents a parallel workload and this application launches this DAG app, mimicking its actual computation and communication requirements of each task.

The *main* function works as a task launcher that creates **threads** to run the tasks and creates the **thread-safe queues** to represent the edges of the DAG. This queue is created for every edge.  Each edge has one sender/receiver; the source task is the only one that writes in the queue, while the target task is the only one that reads from the queue. A multicast-like communication has to be broken down into multiple queues. A queue is named by combining the names of the source and target tasks. For instance, an mq named *n0_n1* connects the tasks *n0* and *n1*. 

***TO REVIEW***

This app currently supports end-to-end DAG deadline checking. This is implemented with POSIX shared memory (shmget and shmat) among the start and the final tasks. Basically, every time the start task starts a new DAG iteration, it saves its start time with *clock_gettime CLOCK_MONOTONIC_RAW* in the shared memory address. The final task computes its current time and subtracts from the time in the shared variable. Since DAG_DEADLINE is lower or equal to the DAG_DEADLINE, there is no risk of synchronization issues among the start and final tasks. This approach has limitations mentioned in the TODO list. 

## Usage example

For example, a DAG like this one:
```
      n1
     /   \
  n0      n3
     \   /
      n2 
```

is described as:

```C
#define N_TASKS 4
#define N_EDGES 4
#define MAX_OUT_EDGES_PER_TASK 2
#define MAX_IN_EDGES_PER_TASK 2
#define MAX_MSG_LEN 256
#define REPETITIONS 40 // the number of iterations of the complete DAG
#define DAG_PERIOD 100000 // in us 
#define DAG_DEADLINE DAG_PERIOD 
unsigned tasks_wcet[N_TASKS] = {10000,18000,15000,5000}; // in us. The actual computation time is decided randomly in runtime
unsigned edge_bytes[N_EDGES] = {20,50,4,8}; // amount of data sent in the queue per DAG period
unsigned adjacency_matrix[N_TASKS][N_TASKS] = {
    {0,1,1,0},
    {0,0,0,1},
    {0,0,0,1},
    {0,0,0,0},
};
```

# How to compile

```
$> mkdir build; cd build
$> cmake ..
$> make -j 6
```

The compilation parameter *LOG_LEVEL* can be used to change verbosity.

# How to run

```
$> ./dag_gen
n0, 10000
 ins: 
 outs: /n0_n1, /n0_n2, 
n1, 18000
 ins: /n0_n1, 
 outs: /n1_n3, 
n2, 15000
 ins: /n0_n2, 
 outs: /n2_n3, 
n3, 5000
 ins: /n1_n3, /n2_n3, 
 outs: 
....
task n0 (0): task duration 99992 us
task n2 (0): task duration 12248 us
task n1 (0): task duration 16403 us
task n3 (0): task duration 4057 us
task n3 (0): dag duration 769786918014 - 769786891087 = 26927 us = 26 ms = 0 s
...
```

The number in parenthesis represents the iteration. Note that, since the tasks are parallel processes, the iteration order when logging can be a bit mixed up. Note also that the 1st task has a WCET of 10000 us but the log says it took 99992 us. This is because the 1st task is the only periodic task in the DAG. So, it's execution time is *max (wcet_0,dag period)*. The execution time of all other tasks starts as soon as they receive all required inputs and ends once they have sent all messages. In other words, the *task duration* accounts for the task computation plus its messages sent. It does not account for the waiting time for incoming messages.

# Design decisions

This application is not an *one-size fits all* solution for modeling DAG-like applications due to the complexities of application-level requirements and different ways to implement it. So, this section briefly explains the mindset behind the decision-making process. 

The main design decision were:
 1. *process-level vs thread-level for task modeling*:
 2. *which IPC to use*:
 3. ....

## Process-level vs thread-level for task modeling

 - modeling tasks as OS processes by using *fork()*. 
   - Once the process is forked, there is no implicit shared memory anymore, requiring explicit communication to transfer data among tasks.
   - Support message passing (i.e. sockets or POSIX message queue) or byte streams (e.g. FIFO, pipes) for explicit communication
   - Shared memory is still possible, but with explicit declarations like POSIX shared memory (.e.g *shm_open()*)
   - Possible to evolve from UMA/NUMA shared memory computer arch to a NORMA-based distributed computing
 - modeling tasks as threads by using *pthread_create()*.
   - Very efficient for shared memory computer arch
   - Limited to shared memory IPC strategies

## Selecting IPC for task communication

Once process-based task modeling was chosen, it gave us more IPC options. 
 - Message-based data transfer: creates a system call but has built-in synchronization. POSIX message queue;
 - Shared-memory comm: has no system call, but requires the programmer to implement the synchronization (e.g. semaphore). E.g.: POSIX shared memory

For more background information, please check Chapter 43: Interprocess Communication Overview, from *The Linux programming interface a Linux and UNIX system programming handbook*, by Michael Kerrisk.

## Summarizing

Process-level task modeling and message-based communication were chosen to provide more flexibility in terms of alternative IPC strategies and computer archs, at the expense of more complexity (and possibly performance penalty) compared to the thread-based, shared-memory approach.

# Comparison with other tools

[rt-app](https://github.com/scheduler-tools/rt-app), as defined by the authors, is a test application that starts multiple periodic threads in order to simulate a real-time periodic load.

*cyclictest* is for testing the OS response time under a certain task set of periodic real-time tasks modeled as [fixed priorities tasks](https://wiki.linuxfoundation.org/realtime/documentation/howto/tools/cyclictest/start) or [deadline-based tasks](https://man.archlinux.org/man/community/rt-tests/cyclicdeadline.8.en).

I hope that, in the future, this tool could do something similar, but the task set being modeled as a DAG, with it's intrinsic task dependencies. 

## Using perf and hotspot

Install perf

```
$ sudo apt install linux-tools-$(uname -r) linux-tools-generic
```

Download [hotspot](https://github.com/KDAB/hotspot/releases/tag/v1.3.0)  


```
$ sudo perf record -o ./perf.data --call-graph dwarf -e 'sched:*' -- app
```

load the perf.dat file into hotspot.

# FAQ

 1. why not use middlewares like DDS, ROS, OpenMP, corba-like, etc ? 
   - These could be future updates, but be aware that they usually have internal scheduling policies on top of the OS scheduling that could complicate realtime analyses. As an initial step, to simplify things, we want to support Linux OS-only resources, with no external dependencies. 
 2. ... 

# To read 

 - https://wang-yimu.com/a-tutorial-on-shared-memory-inter-process-communication/
 - https://github.com/SnowWalkerJ/shared_memory/blob/master/src/ShmMessageQueue.h  this is similar to what i want, but it uses boost::interprocess
 - https://stackoverflow.com/questions/22207546/shared-memory-ipc-synchronization-lock-free
 - https://github.com/ashishsony/dev/blob/master/c++ConcurrencyInAction/threadsafeQueue.cpp
 - https://gist.github.com/carlchen0928/0d0d9e8f59728a4872f3
 - https://italiancoders.it/the-queue-come-scrivere-una-thread-safe-queue-in-c-c-e-go/
 - https://gist.github.com/Kuxe/6bdd5b748b5f11b303a5cccbb8c8a731  thread-safe queue w semaphore

# references

 - [Beej's Guide to Unix IPC](https://beej.us/guide/bgipc/html/single/bgipc.html)

# TODO

 - the amount of data sent in the messages still don't correspond to the DAG description;
 - it seems to have some sync issue among the tasks. A temporary hack is to put some sleeps when the tasks are spawned;
 - Improve end-to-end deadline checking: there is a potential sync error in the current implementation. If the DAG deadline is violated, the start time would start the next iteration, updating the shared variable. This way, the final task would loose the starting time of the previous iteration, missing the deadline violation. A queue of size one with blocking send could be a solution ?!?!
 - extend the data structure to pin down a task to a core;
 - extend the data structure to set the frequency of the islands;
 - check the power budget;
 - implement thread-level task modeling;
 - implement shared-memory IPC strategy.

## Authors

 - Alexandre Amory (June 2022), [Real-Time Systems Laboratory (ReTiS Lab)](https://retis.santannapisa.it/), [Scuola Superiore Sant'Anna (SSSA)](https://www.santannapisa.it/), Pisa, Italy.

## Funding
 
This software package has been developed in the context of the [AMPERE project](https://ampere-euproject.eu/). This project has received funding from the European Union’s Horizon 2020 research and innovation programme under grant agreement No 871669.
