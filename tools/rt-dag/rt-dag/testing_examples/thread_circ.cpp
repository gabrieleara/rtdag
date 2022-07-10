/**
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
#include <iostream>

#include "circular_buffer.h"
#include "shared_mem_type.h"

// change here the supported amount of buffering.  
circular_buffer <shared_mem_type,3> bq;

void exit_all(int sigid){
    printf("Exting\n");
    exit(0);
}

void writer(){
    printf("[write] starting ...\n");
    shared_mem_type message;

    for (unsigned i = 0; i < 10; ++i) {
        message.set("writer",i);
        bq.push(message);
        std::cout << "[write] (" << i << "), message: " << message.get() << std::endl;
        usleep(1000 * 20);
    }
    printf("[write] sleeping for 1 sec\n");
    usleep(1000 * 1000 * 1);
}

void reader(void){
    unsigned count = 0;
    shared_mem_type message;

    while (true) {
        // TODO: Large stack varible doesn't work in multi-thread env??????
        bq.pop(message);
        std::cout << "[reader] (" << count << "), message: " << message.get() << std::endl;
        count++;
        // reader is slower than writer
        usleep(1000 * 500);
    }
}

int main(int argc, char* argv[])
{
    signal(SIGKILL,exit_all);
    signal(SIGSEGV,exit_all);
    signal(SIGINT,exit_all);

    std::thread t1{writer};
    std::thread t2{reader};
    t1.join();
    t2.join();

    printf("[main] finishing ...\n");

    return 0;
}
