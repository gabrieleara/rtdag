#ifndef INPUT_HEADER_H_
#define INPUT_HEADER_H_

/*
This is a implementation of the input_base class that reads from dag.h.

This was the 1st data input format developed for rt-dag. It has some advantages
like the possibility to optimize code due to use of consts and defines.
However, it required to recompile the rt-dag for every new scenario.
When dealing with a more complex setup where it's required to run hundreds of
scenarios, this becomes cumbersome. It gets even more complex because it
requires cross-compilation to build all the required scenarios
*/

#include "dag.h"
#include "input_base.h"

using namespace std;

class input_header : public input_base {

public:
    input_header(const char *fname_) : input_base() {}

    const char *get_dagset_name() const override {
        return dagset_name;
    }

    unsigned get_n_tasks() const override {
        return N_TASKS;
    }

    unsigned get_n_edges() const override {
        return N_EDGES;
    }

    unsigned get_n_cpus() const {
        return N_CPUS;
    }

    unsigned get_max_out_edges() const override {
        return MAX_OUT_EDGES_PER_TASK;
    }

    unsigned get_max_in_edges() const override {
        return MAX_IN_EDGES_PER_TASK;
    }

    unsigned get_msg_len() const override {
        return MAX_MSG_LEN;
    }

    unsigned get_repetitions() const override {
        return REPETITIONS;
    }

    unsigned long get_period() const override {
        return DAG_PERIOD;
    }

    unsigned long get_deadline() const override {
        return DAG_DEADLINE;
    }

    unsigned long get_hyperperiod() const override {
        return HYPERPERIOD;
    }
    const char *get_tasks_name(unsigned t) const override {
        return tasks_name[t];
    }
    const char *get_tasks_type(unsigned) const override {
        // FIXME: implement me if used
        return NULL;
    }
    int get_fred_id(unsigned) const override {
        // implement me if used
        return -1;
    }

    unsigned long get_tasks_runtime(unsigned t) const override {
        // FIXME: implement me if used
        return 0;
    }

    unsigned long get_tasks_wcet(unsigned t) const override {
        return tasks_wcet[t];
    }

    unsigned long get_tasks_rel_deadline(unsigned t) const {
        return tasks_rel_deadline[t];
    }
    int get_tasks_affinity(unsigned t) const override {
        return task_affinity[t];
    }

    unsigned get_adjacency_matrix(unsigned t1, unsigned t2) const override {
        return adjacency_matrix[t1][t2];
    }

    static constexpr bool has_input_file = false;
};

#endif // INPUT_HEADER_H_
