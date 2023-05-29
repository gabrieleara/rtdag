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
#include <string>

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

vet_cbuffer bq(4);

void exit_all(int sigid){
    printf("Exting\n");
    exit(0);
}

void task(const char * name, 
    const vet_cbuffer& writers,
    const vet_cbuffer& readers){
    printf("[%s] starting ...\n", name);
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
        for (unsigned r=0;r<readers.size();++r){
            readers[r]->pop(data);
            printf("[%s] (%u), received msg: %s\n", name, i, data.get());
        }

        // const unsigned wcet = 1000 * 20;
        // rnd_val = zeroToOne(engine);
        // execution_time = 4.0f/5.0f* wcet + 1.0f/5.0f*rnd_val;
        // usleep(execution_time);
        // usleep(wcet);
        if (readers.size()==0){
            // teh start task is much faster to make sure that will not be data overrun
            usleep(1000 * 20);
        }else{
            usleep(1000 * 500);
        }
        for (unsigned w=0;w<writers.size();++w){
            data.set(name, i);
            writers[w]->push(data);
            printf("[%s] (%u), sent msg: %s\n", name, i, data.get());
        }

    }
    printf("[%s] sleeping for 1 sec\n",name);
    usleep(1000 * 1000 * 1);
}

int main(int argc, char* argv[])
{
    signal(SIGKILL,exit_all);
    signal(SIGSEGV,exit_all);
    signal(SIGINT,exit_all);

    bq[0] = (ptr_cbuffer)new cbuffer;
    bq[1] = (ptr_cbuffer)new cbuffer;
    bq[2] = (ptr_cbuffer)new cbuffer;
    bq[3] = (ptr_cbuffer)new cbuffer;

/*
      n1
     /   \
  n0      n3
     \   /
      n2 
*/
    vet_cbuffer n0_r;
    vet_cbuffer n0_w;
    vet_cbuffer n1_r;
    vet_cbuffer n1_w;
    vet_cbuffer n2_r;
    vet_cbuffer n2_w;
    vet_cbuffer n3_r;
    vet_cbuffer n3_w;
    n0_w.push_back(bq[0]);
    n0_w.push_back(bq[1]);
    n1_r.push_back(bq[0]);
    n1_w.push_back(bq[2]);
    n2_r.push_back(bq[1]);
    n2_w.push_back(bq[3]);
    n3_r.push_back(bq[2]);
    n3_r.push_back(bq[3]);

    std::thread n0{task,"n0",n0_w,n0_r};
    std::thread n1{task,"n1",n1_w,n1_r};
    std::thread n2{task,"n2",n2_w,n2_r};
    std::thread n3{task,"n3",n3_w,n3_r};

    n0.join();
    n1.join();
    n2.join();
    n3.join();

    printf("[main] finishing ...\n");

    return 0;
}
