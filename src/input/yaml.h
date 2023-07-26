#ifndef RTDAG_INPUT_YAML_H
#define RTDAG_INPUT_YAML_H

/*
This is a implementation of the input_base class that reads the dag
configuration from a YAML file.

despite input_header, this approach does not required to recompile the rt-dag
for every new scenario. When dealing with a more complex setup where it's
required to run hundreds of scenarios, this becomes cumbersome. It gets even
more complex because it requires cross-compilation to build all the required
scenarios
*/

#include "input/base.h"
#include "time_aux.h"
#include "newstuff/mqueue.h"

#include <string>
#include <vector>
#include <limits>
#include <yaml-cpp/yaml.h>

#define MAX_N_TASKS (MultiQueue::mask_type().size())

class input_yaml : public input_base {
private:
    // YAML Structure:
    //
    // hyperperiod: long # in us
    // repetitions: int
    //
    // n_cpus: int
    // cpus_freq: int[] # in MHz
    //
    // dag_name: std::string
    // n_edges: int
    // max_out_edges: int
    // max_in_edges: int
    // max_msg_len: int
    //
    // dag_period: long # in us
    // dag_deadline: long # in us
    //
    // n_tasks: int
    // tasks_name: std::string[], one per task
    // tasks_type: std::string[], one per task
    // tasks_wcet: long[] # in us
    // tasks_runtime: long[] # in us
    // tasks_rel_deadline: long[] # in us
    // tasks_affinity: int[]
    // fred_id: int[] # -1 if no fred id
    //
    // # NOTE: there are other attributes not represented in this comment now!
    //
    // adjacency_matrix: int[][]
    //
    // # (NOTE: sum of the longest path deadlines MUST be <= dag_deadline)
    // # NOTE: (!=0 means link from task l (line) to task c (column))

    // ----------------- EXPERIMENT DATA -----------------
    long long hyperperiod;
    int repetitions;

    // We do not care of accessing these fast, we can accept
    // vector's double indirection and gain flexibility in
    // the number of CPUs
    std::vector<int> cpu_freqs;

    // -------------------- DAG DATA ---------------------

    std::string dag_name;
    int n_edges;
    int max_out_edges;
    int max_in_edges;
    int max_msg_len;

    long long dag_period;
    long long dag_deadline;

    // ------------------- TASKS DATA --------------------

    struct task_data {
        std::string name;
        std::string type;
        int prio;
        long long wcet;
        long long runtime;
        long long rel_deadline;
        int affinity;
        int matrix_size;
        int omp_target = 0;
        float ticks_per_us = -1;
        float expected_wcet_ratio = 1;
#if RTDAG_FRED_SUPPORT == ON
        int fred_id;
#endif
    };

    // Stack-allocated, for fast access, we are tied to
    // the maximum number of tasks
    template <typename T, size_t N>
    using square_matrix = std::array<std::array<T, N>, N>;

    int n_tasks;
    std::array<task_data, MAX_N_TASKS> tasks;
    square_matrix<int, MAX_N_TASKS> adjacency_matrix;

public:
    input_yaml(const char *fname);

    const char *get_dagset_name() const override {
        return dag_name.c_str();
    }

    unsigned get_n_tasks() const override {
        return n_tasks;
    }

    unsigned get_n_edges() const override {
        // FIXME: there are no checks that this number is correct
        return n_edges;
    }

    unsigned get_n_cpus() const override {
        return cpu_freqs.size();
    }

    unsigned get_max_out_edges() const override {
        return max_out_edges;
    }

    unsigned get_max_in_edges() const override {
        return max_in_edges;
    }

    unsigned get_msg_len() const override {
        return max_msg_len;
    }

    unsigned get_repetitions() const override {
        return repetitions;
    }

    unsigned long get_period() const override {
        return dag_period;
    }

    unsigned long get_deadline() const override {
        return dag_deadline;
    }

    unsigned long get_hyperperiod() const override {
        return hyperperiod;
    }
    const char *get_tasks_name(unsigned t) const override {
        return tasks[t].name.c_str();
    }
    const char *get_tasks_type(unsigned t) const override {
        return tasks[t].type.c_str();
    }

#if RTDAG_FRED_SUPPORT == ON
    int get_fred_id(unsigned t) const override {
        return tasks[t].fred_id;
    }
#endif

    unsigned int get_tasks_prio(unsigned t) const override {
        return tasks[t].prio;
    }

    unsigned long get_tasks_runtime(unsigned t) const override {
        return tasks[t].runtime;
    }

    unsigned long get_tasks_wcet(unsigned t) const override {
        return tasks[t].wcet;
    }

    unsigned long get_tasks_rel_deadline(unsigned t) const override {
        return tasks[t].rel_deadline;
    }

    int get_tasks_affinity(unsigned t) const override {
        return tasks[t].affinity;
    }

    unsigned get_adjacency_matrix(unsigned t1, unsigned t2) const override {
        return adjacency_matrix[t1][t2];
    }

    float get_tasks_expected_wcet_ratio(unsigned t) const override {
        return tasks[t].expected_wcet_ratio; // FIXME: this may be std::optional
    }

    unsigned int get_matrix_size(unsigned t) const override {
        return tasks[t].matrix_size;
    }

    unsigned int get_omp_target(unsigned t) const override {
        return tasks[t].omp_target;
    }

    float get_ticks_per_us(unsigned t) const override {
        float v = tasks[t].ticks_per_us;
        // Return global variable if value is not supplied (basically if
        // the value is positive it overrides the global variable)
        return v > 0 ? v : ticks_per_us;
    }

public:
    static constexpr bool has_input_file = true;
};

#endif // RTDAG_INPUT_YAML_H
