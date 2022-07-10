/**
 * @file thread.cpp
 * @author Alexandre Amory
 * @brief Example of usage of thread-safe queue with blocking push once the queue size limite is reached
 * @version 0.1
 * @date 2022-07-06
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <unistd.h>
#include <csignal>
#include <thread>
#include <vector>
#include <iostream>
//#include <random>
#include <string>
#include <cstring>

#include "circular_buffer.h"
#include "shared_mem_type.h"

/*
implement 4 queues, for a communication pattern like this
      n1
     /   \
  n0      n3
     \   /
      n2 
*/
#define BUFFER_LINES 3

using cbuffer = circular_buffer <shared_mem_type,BUFFER_LINES>;
using ptr_cbuffer = std::shared_ptr< cbuffer >;
using vet_cbuffer = std::vector< ptr_cbuffer >;

typedef struct {
    cbuffer buff;
    unsigned size;
    char name[32];
} edge_type;

using ptr_edge = std::shared_ptr< edge_type >;

typedef struct {
    std::string name;
    unsigned wcet;
    std::vector< ptr_edge > in_buffers;
    std::vector< ptr_edge > out_buffers;
} task_type;

std::vector< task_type > tasks;


void exit_all(int sigid){
    printf("Exting\n");
    exit(0);
}


void  print_task_cfg(){
    unsigned i,c;
  for(i=0;i<4;++i){
    std::cout << tasks[i].name << ", " << tasks[i].wcet << std::endl;
    std::cout << " ins: ";
    for(c=0;c<tasks[i].in_buffers.size();++c)
        std::cout << tasks[i].in_buffers[c]->name << "(" << tasks[i].in_buffers[c]->size << "), ";
    std::cout << std::endl;
    std::cout << " outs: ";
    for(c=0;c<tasks[i].out_buffers.size();++c)
        std::cout << tasks[i].out_buffers[c]->name << "(" << tasks[i].out_buffers[c]->size << "), ";
    std::cout << std::endl;
  }
}

void task(const task_type & task){
    printf("[%s] starting ...\n", task.name.c_str());
    shared_mem_type data;
    // unsigned seed = 1234;
    // unsigned long execution_time;
    // float rnd_val;

    // // each task has its own rnd engine to limit blocking for shared resources.
    // // the thread seed is built by summin up the main seed + a hash of the task name, which is unique
    // seed+= std::hash<std::string>{}(name);
    // std::mt19937_64 engine(static_cast<uint64_t> (seed));
    // std::uniform_real_distribution<double> zeroToOne(0.0, 1.0);

    for (unsigned i = 0; i < 5; ++i) {
        for (unsigned r=0;r<task.in_buffers.size();++r){
            task.in_buffers[r]->buff.pop(data);
            printf("[%s] (%u), received msg: %s\n", task.name.c_str(), i, data.get());
        }

        // const unsigned wcet = 1000 * 20;
        // rnd_val = zeroToOne(engine);
        // execution_time = 4.0f/5.0f* wcet + 1.0f/5.0f*rnd_val;
        // usleep(execution_time);
        // usleep(wcet);
        usleep(1000 * task.wcet);

        for (unsigned w=0;w<task.out_buffers.size();++w){
            data.set(task.name.c_str(), i);
            task.out_buffers[w]->buff.push(data);
            printf("[%s] (%u), sent msg: %s\n", task.name.c_str(), i, data.get());
        }

    }
    printf("[%s] sleeping for 1 sec\n",task.name.c_str());
    usleep(1000 * 1000 * 1);
}

int main(int argc, char* argv[])
{
    signal(SIGKILL,exit_all);
    signal(SIGSEGV,exit_all);
    signal(SIGINT,exit_all);


    tasks.resize(4);
    // task description
    tasks[0].name = "n0";
    tasks[0].wcet = 50;
    tasks[0].out_buffers.resize(2);

    tasks[1].name = "n1";
    tasks[1].wcet = 500;
    tasks[1].in_buffers.resize(1);
    tasks[1].out_buffers.resize(1);

    tasks[2].name = "n2";
    tasks[2].wcet = 200;
    tasks[2].in_buffers.resize(1);
    tasks[2].out_buffers.resize(1);

    tasks[3].name = "n3";
    tasks[3].wcet = 100;
    tasks[3].in_buffers.resize(2);


/* edge description
      n1
     /   \
  n0      n3
     \   /
      n2 
*/
    ptr_edge new_edge;
    new_edge = (ptr_edge) new edge_type;
    strcpy(new_edge->name, "n0_n1");
    new_edge->size = 30;
    tasks[0].out_buffers[0] = new_edge;
    tasks[1].in_buffers[0] = new_edge;

    new_edge = (ptr_edge) new edge_type;
    strcpy(new_edge->name, "n0_n2");
    new_edge->size = 50;
    tasks[0].out_buffers[1] = new_edge;
    tasks[2].in_buffers[0] = new_edge;

    new_edge = (ptr_edge) new edge_type;
    strcpy(new_edge->name, "n1_n3");
    new_edge->size = 20;
    tasks[1].out_buffers[0] = new_edge;
    tasks[3].in_buffers[0] = new_edge;

    new_edge = (ptr_edge) new edge_type;
    strcpy(new_edge->name, "n2_n3");
    new_edge->size = 10;
    tasks[2].out_buffers[0] = new_edge;
    tasks[3].in_buffers[1] = new_edge;        

    print_task_cfg();

    std::thread n0{task,tasks[0]};
    std::thread n1{task,tasks[1]};
    std::thread n2{task,tasks[2]};
    std::thread n3{task,tasks[3]};

    n0.join();
    n1.join();
    n2.join();
    n3.join();

    printf("[main] finishing ...\n");

    return 0;
}
