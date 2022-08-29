
/*
 * Simplified instance of main.cpp used to test the task sync. Used for testing purposes until i get into final design.
 * 
 * Author: 
 *  Alexandre Amory (June 2022), ReTiS Lab, Scuola Sant'Anna, Pisa, Italy.
 * 
 */

#include <iostream>           // std::cout
#include <thread>             // std::thread
#include <vector>
#include <random>
#include <chrono>
#include <sstream>
#include <string>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <unistd.h> // getpid

#include "circular_buffer.h"
# include "shared_mem_type.h"

// the dag definition is here
# include "dag.h"

using namespace std;

typedef struct {
    //unique_ptr< circular_buffer<shared_mem_type,1> > buff;
    circular_buffer<shared_mem_type,1> buff;
    unsigned size;
    char name[32];
} edge_type;

typedef struct {
    string name;
    unsigned wcet;
    // a single object of type edge_type is shared between the sender and receiver
    vector< shared_ptr <edge_type> > in_buffers;
    vector< shared_ptr <edge_type> > out_buffers;
} task_type;

////////////////////////////
// globals used by the tasks
////////////////////////////
unsigned seed;
// the task definition
vector< task_type > tasks;

void print_task_cfg();

///////////////////////////////////////
// describing each task of the dag
///////////////////////////////////////
// 'period_ns' argument is only used when the task is periodic, which is tipically only the first tasks of the DAG
// all tasks have the same signature to simplify automatic codegen in the future
void task_creator(const unsigned seed, const task_type& task, const unsigned period_ns=0){
  unsigned iter=0;
  unsigned i;
  char task_name[32];
  strcpy(task_name, task.name.c_str());
  assert((period_ns != 0 && period_ns>task.wcet) || period_ns == 0);

  std::hash<std::thread::id> myHashObject{};
  uint32_t threadID = myHashObject(std::this_thread::get_id());
  printf("task %s created: pid = %u, ppid = %d\n", task_name, threadID, getppid());
  shared_mem_type message;
  while(iter < REPETITIONS){
    // wait all incomming messages
    printf("task %s (%u): waiting msgs\n", task_name, iter);
    for(i=0;i<task.in_buffers.size();++i){
        printf("task %s (%u), waiting buffer %s(%d)\n", task_name, iter, task.in_buffers[i]->name, task.in_buffers[i]->size);
        task.in_buffers[i]->buff.pop(message);
        printf("task %s (%u), buffer %s: got message: %s\n", task_name, iter, task.in_buffers[i]->name, message.get());
    }

    usleep(1000 * task.wcet);

    // send data to the next tasks
    printf("task %s (%u): sending msgs!\n", task_name,iter);
    for(i=0;i<task.out_buffers.size();++i){
        message.set(task_name,i);
        task.out_buffers[i]->buff.push(message);
        printf("task %s (%u): buffer %s, sent message: '%s'\n",task_name, iter, task.out_buffers[i]->name, message.get());
    }
    printf("task %s (%u): all msgs sent!\n", task_name, iter);
    ++iter;
  }

}


void  print_task_cfg(){
    unsigned i,c;
  for(i=0;i<N_TASKS;++i){
    cout << tasks[i].name << ", " << tasks[i].wcet << endl;
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

int main() {

  // uncomment this to get a randon seed
  //unsigned seed = time(0);
  // or set manually a constant seed to repeat the same sequence
  seed = 123456;
  cout << "SEED: " << seed << endl;  
  
  tasks.resize(N_TASKS);
  int i,c;

  for(i=0;i<N_TASKS;++i){
    tasks[i].name = "n"+to_string(i);
    tasks[i].wcet = tasks_wcet[i];
    // create the edges/queues w unique names
    for(c=0;c<N_TASKS;++c){
        if (adjacency_matrix[i][c]!=0){
            // TODO: the edges are now implementing 1:1 communication, 
            // but it would be possible to have multiple readers
            //new_edge = new edge_type;
            std::shared_ptr<edge_type> new_edge(new edge_type);
            snprintf(new_edge->name, sizeof(new_edge->name), "n%u_n%u", i,c);
            printf("creating buffer '%s' of size %u\n", new_edge->name, adjacency_matrix[i][c]);
            new_edge->size = adjacency_matrix[i][c]+1;
            tasks[i].out_buffers.push_back(new_edge);
            tasks[c].in_buffers.push_back(new_edge);
        }
    }
  }
  print_task_cfg();

  vector<std::thread> threads;
  threads.push_back(thread(task_creator,seed, tasks[0], DAG_PERIOD));
  printf("[main] pid %d task 0\n", getpid());
  for (unsigned i = 1; i < N_TASKS; i++) {
    std::this_thread::sleep_for (std::chrono::milliseconds(10));
    threads.push_back(std::thread(task_creator, seed, tasks[i], 0));
    printf("[main] pid %d task %d\n", getpid(), i);
  }
 
  for (auto &th : threads) {
    th.join();
  }

  printf("[main] finished spawning tasks...\n");

  return 0;
}
