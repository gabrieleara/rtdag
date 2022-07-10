/**
 * @author Alexandre Amory
 * @brief writer example of shared memmory IPC with blocking push once the queue size limite is reached
 * @version 0.1
 * @date 2022-07-06
 * 
 * @copyright Copyright (c) 2022
 */
//#include <unistd.h>
#include <csignal>
#include <iostream>

#include "circular_shm.h"
#include "shared_mem_type.h"

void exit_all(int sigid){
    printf("Exting\n");
    exit(0);
}

int main(int argc, char* argv[]){
    
    signal(SIGKILL,exit_all);
    signal(SIGSEGV,exit_all);
    signal(SIGINT,exit_all);

    std::cout << "[writer] starting ..." << std::endl;
    shared_mem_type message;
    // change here the supported amount of buffering.  
    circular_shm <shared_mem_type,3> bq("my_shm");

    for (unsigned i = 0; i < 10; ++i) {
        message.set("writer",i);
        bq.push(message);
        std::cout << "[write] sent token: " << i << ", message: " << message.get() << std::endl;
        usleep(1000 * 20);
    }
    std::cout << "[write] sleeping for 1 sec" << std::endl;
    usleep(1000 * 1000 * 1);

    return 0;
}
