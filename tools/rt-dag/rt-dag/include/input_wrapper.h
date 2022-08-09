#ifndef INPUT_WRAPPER_H_
#define INPUT_WRAPPER_H_

/*
This is a wrapper class to ease the integration of multiple input formats for rt-dag.
TODO: currently not used due to failed attempt to use the base class.

*/

#include <vector>
#include <memory>

using namespace std;
/*
class cbuffer;
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
*/

class input_wrapper{
protected:
    // used only when some file is read
    const char * fname; 
    // task connectivity info
    //vector< task_type > tasks;

public:

    input_wrapper(const char* fname_): fname(fname_) {
        /*
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
        */
    }

    virtual const char *    get_dagset_name() const = 0;
    virtual unsigned  get_n_tasks() const = 0;
    virtual unsigned  get_n_edges() const  = 0;
    virtual unsigned  get_max_out_edges() const  = 0;
    virtual unsigned  get_max_in_edges() const  = 0;
    virtual unsigned  get_msg_len() const  = 0;
    virtual unsigned  get_repetitions() const  = 0;
    virtual unsigned long  get_period() const  = 0;
    virtual unsigned long  get_deadline() const  = 0;
    virtual const char *    get_tasks_name(unsigned t) const = 0;
    virtual unsigned long  get_tasks_wcet(unsigned t) const = 0;
    virtual unsigned long  get_tasks_rel_deadline(unsigned t) const = 0;
    virtual unsigned  get_tasks_affinity(unsigned t) const = 0;
    virtual unsigned  get_adjacency_matrix(unsigned t1,unsigned t2) const = 0;

};

#endif // INPUT_WRAPPER_H_
