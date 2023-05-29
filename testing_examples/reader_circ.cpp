/**
 * @author Alexandre Amory
 * @brief reader example of shared memmory IPC with blocking push once the queue size limite is reached
 * @version 0.1
 * @date 2022-07-06
 * 
 * @copyright Copyright (c) 2022
 */
#include <unistd.h>
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

    unsigned count = 0;
    std::cout << "[reader3] starting ..." << std::endl;
    shared_mem_type message;
    // change here the supported amount of buffering.  
    circular_shm <shared_mem_type,3> bq("my_shm");

    while (true) {
        bq.pop(message);
        std::cout << "[reader] (" << count << "), message: " << message.get() << std::endl;
        count++;
        // reader is slower than writer
        usleep(1000 * 500);
    }
    std::cout << "pass test" << std::endl;

    return 0;
}
