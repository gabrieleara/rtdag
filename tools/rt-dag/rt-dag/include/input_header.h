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

#include "input_wrapper.h"
#include "dag.h"

class input_header: public input_wrapper{
public:

input_header(const char* fname_): input_wrapper(fname_) {}

const char *    get_dagset_name() const { return dagset_name;}
const unsigned  get_n_tasks() const { return N_TASKS;}
const unsigned  get_n_edges() const { return N_EDGES;}
const unsigned  get_max_out_edges() const { return MAX_OUT_EDGES_PER_TASK;}
const unsigned  get_max_in_edges() const { return MAX_IN_EDGES_PER_TASK;}
const unsigned  get_msg_len() const { return MAX_MSG_LEN;}
const unsigned  get_repetitions() const { return REPETITIONS;}
const unsigned long get_period() const { return DAG_PERIOD;}
const unsigned long get_deadline() const { return DAG_DEADLINE;}
const char *    get_tasks_name(unsigned t) const { return tasks_name[t];}
const unsigned long  get_tasks_wcet(unsigned t) const { return tasks_wcet[t];}
const unsigned long  get_tasks_rel_deadline(unsigned t) const{ return tasks_rel_deadline[t];}
const unsigned  get_tasks_affinity(unsigned t) const { return task_affinity[t];}
const unsigned  get_adjacency_matrix(unsigned t1,unsigned t2) const { return adjacency_matrix[t1][t2];}

};

#endif // INPUT_HEADER_H_
