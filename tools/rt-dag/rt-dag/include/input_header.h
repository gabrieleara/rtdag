#ifndef INPUT_HEADER_H_
#define INPUT_HEADER_H_

/*
This is a wrapper classe that reads from dag.h. This is made like this to 
ease the integration of multiple input formats for rt-dag.

This was the 1st data input format developed for rt-dag. It has some advantages
like the possibility to optimize code due to use of consts and defines.
However, it required to recompile the rt-dag for every new scenario.
When dealing with a more complex setup where it's required to run hundreds of scenarios,
this becomes cumbersome. It gets even more complex because it requires cross-compilation
to build all the required scenarios
*/

// TODO: implemente a proper base class 
//#include "input_wrapper.h"
#include <memory>
#include <vector>
#include "dag.h"

using namespace std;

//class cbuffer;
// task_type and edge_type provides a better interface to get dag connetivity data
typedef struct {
    // the only reason this is pointer is that, when using circular_shm, it requries to pass the shared mem name
    std::unique_ptr< cbuffer > buff;
    unsigned size; // in bytes
    char name[32];
} edge_type;

using ptr_edge = std::shared_ptr< edge_type >;

typedef struct {
    vector< ptr_edge > in_buffers;
    vector< ptr_edge > out_buffers;
} task_type;


// class input_header: public input_wrapper{
class input_header{

public:
    // task connectivity info
    vector< task_type > tasks;

    // input_header(const char* fname_): input_wrapper(fname_) {}
    input_header(const char* fname_){
        unsigned i,c;
        tasks.resize(get_n_tasks());
        for(i=0;i<get_n_tasks();++i){
            // create the edges/queues w unique names
            for(c=0;c<get_n_tasks();++c){
                if (get_adjacency_matrix(i,c)!=0){
                    // TODO: the edges are now implementing 1:1 communication, 
                    // but it would be possible to have multiple readers
                    ptr_edge new_edge(new edge_type);
                    snprintf(new_edge->name, 32, "n%u_n%u", i,c);
                    // TODO
                    //new_edge->buff = (std::unique_ptr< cbuffer >) new cbuffer(new_edge->name);
                    // this message size includes the string terminator, thus, threre is no +1 here
                    new_edge->size = get_adjacency_matrix(i,c);
                    tasks[i].out_buffers.push_back(new_edge);
                    tasks[c].in_buffers.push_back(new_edge);
                }
            }
        }
    }

    const char *    get_dagset_name() const { return dagset_name;}
    unsigned  get_n_tasks() const { return N_TASKS;}
    unsigned  get_n_edges() const { return N_EDGES;}
    unsigned  get_max_out_edges() const { return MAX_OUT_EDGES_PER_TASK;}
    unsigned  get_max_in_edges() const { return MAX_IN_EDGES_PER_TASK;}
    unsigned  get_msg_len() const { return MAX_MSG_LEN;}
    unsigned  get_repetitions() const { return REPETITIONS;}
    unsigned long get_period() const { return DAG_PERIOD;}
    unsigned long get_deadline() const { return DAG_DEADLINE;}
    const char *    get_tasks_name(unsigned t) const { return tasks_name[t];}
    unsigned long  get_tasks_wcet(unsigned t) const { return tasks_wcet[t];}
    unsigned long  get_tasks_rel_deadline(unsigned t) const{ return tasks_rel_deadline[t];}
    unsigned  get_tasks_affinity(unsigned t) const { return task_affinity[t];}
    unsigned  get_adjacency_matrix(unsigned t1,unsigned t2) const { return adjacency_matrix[t1][t2];}

};

#endif // INPUT_HEADER_H_
