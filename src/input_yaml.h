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

#include "input_base.h"
#include "time_aux.h"

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

// FIXME: The number of 32 is tied to the flags in the
// multi_queue (and it's not even technically correct for
// platforms in which an int/long is not 32 bits...)
#define MAX_N_TASKS 32

enum class yaml_error_type {
    YAML_WARN,
    YAML_ERROR,
};

template <yaml_error_type...>
constexpr std::false_type always_false{};

template <yaml_error_type error>
static inline constexpr const char *get_error_msg() {
    if constexpr (error == yaml_error_type::YAML_ERROR) {
        return "ERROR";
    } else if constexpr (error == yaml_error_type::YAML_WARN) {
        return "WARN";
    } else {
        static_assert(always_false<error>, "Unexpected yaml_error_type!");
    }

    // Should have never been taken!
}

template <yaml_error_type error>
static inline void exit_if_fatal_error() {
    if constexpr (error == yaml_error_type::YAML_ERROR) {
        std::exit(EXIT_FAILURE);
    } else if constexpr (error == yaml_error_type::YAML_WARN) {
        // Do nothing
    } else {
        static_assert(always_false<error>, "Unexpected yaml_error_type!");
    }
}

template <class T, yaml_error_type error = yaml_error_type::YAML_WARN>
static inline auto get_attribute(const YAML::Node &in, const char *attr,
                                 const char *fname, T default_v = {}) {
    if (!in[attr]) {
        std::fprintf(stderr, "%s: missing attribute '%s' in input file %s.\n",
                     get_error_msg<error>(), attr, fname);
        exit_if_fatal_error<error>();
        return default_v;
    }

    // TODO: catch conversion errors and print something nicer...
    return in[attr].as<T>();
}

static inline YAML::Node read_yaml_file(const char *fname) {
    try {
        return YAML::LoadFile(fname);
    } catch (YAML::ParserException &e) {
        std::fprintf(stderr, "Exception: %s\n", e.what());
        std::fprintf(stderr, "ERROR: Syntax error file %s\n", fname);
        std::exit(EXIT_FAILURE);
    }
}

template <yaml_error_type error>
static inline void exact_length(int expected, size_t size,
                                const std::string &name) {
    if (expected < 0 || unsigned(expected) != size) {
        fprintf(stderr,
                "%s: attribute %s has wrong size: %d expected, found %lu\n",
                get_error_msg<error>(), name.c_str(), expected, size);
        exit_if_fatal_error<error>();
    }
}

class input_yaml : public input_base {
private:
    using string = std::string;

    // YAML Structure:
    //
    // hyperperiod: long # in us
    // repetitions: int
    //
    // n_cpus: int
    // cpus_freq: int[] # in MHz
    //
    // dag_name: string
    // n_edges: int
    // max_out_edges: int
    // max_in_edges: int
    // max_msg_len: int
    //
    // dag_period: long # in us
    // dag_deadline: long # in us
    //
    // n_tasks: int
    // tasks_name: string[], one per task
    // tasks_type: string[], one per task
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

    string dag_name;
    int n_edges;
    int max_out_edges;
    int max_in_edges;
    int max_msg_len;

    long long dag_period;
    long long dag_deadline;

    // ------------------- TASKS DATA --------------------

    struct task_data {
        string name;
        string type;
        long long prio;
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
    input_yaml(const char *fname) : input_base() {
        YAML::Node input = read_yaml_file(fname);

#define M_GET_ATTR(dest, attr)                                                 \
    (dest = get_attribute<decltype(dest), yaml_error_type::YAML_ERROR>(        \
         input, attr, fname))

        M_GET_ATTR(repetitions, "repetitions");
        M_GET_ATTR(hyperperiod, "hyperperiod");
        M_GET_ATTR(cpu_freqs, "cpus_freq");

        int n_cpus;
        M_GET_ATTR(n_cpus, "n_cpus");
        exact_length<yaml_error_type::YAML_WARN>(n_cpus, cpu_freqs.size(),
                                                 "cpus_freq");

        M_GET_ATTR(dag_name, "dag_name");
        M_GET_ATTR(n_edges, "n_edges");
        M_GET_ATTR(max_out_edges, "max_out_edges");
        M_GET_ATTR(max_in_edges, "max_in_edges");
        M_GET_ATTR(max_msg_len, "max_msg_len");

        M_GET_ATTR(dag_period, "dag_period");
        M_GET_ATTR(dag_deadline, "dag_deadline");

        M_GET_ATTR(n_tasks, "n_tasks");
        if (n_tasks < 1) {
            std::fprintf(stderr, "ERROR: negative value in 'n_tasks'\n");
            std::exit(EXIT_FAILURE);
        }

        if (n_tasks > MAX_N_TASKS) {
            std::fprintf(
                stderr,
                "ERROR: 'n_tasks' greater than maximum allowed: %d > %d\n",
                n_tasks, MAX_N_TASKS);
            std::fprintf(stderr, "Please, recompile rtdag to increase the "
                                 "maximum allowed number.\n");
            std::exit(EXIT_FAILURE);
        }

        std::vector<string> task_names;
        std::vector<string> task_types;
        std::vector<long long> task_prios;
        std::vector<long long> task_wcets;
        std::vector<long long> task_runtimes;
        std::vector<long long> task_rel_deadlines;
        std::vector<int> task_affinities;
        std::vector<int> task_matrix_size;
        std::vector<float> task_ticks_us;
        std::vector<float> task_ewr;

        // Optional per-task attributes:
        std::vector<int> task_omp_target;
        std::vector<std::vector<int>> adj_mat;
        std::vector<int> task_matrix_size_default(n_tasks, 4);
        std::vector<int> task_omp_target_default(n_tasks, 0);
        std::vector<float> task_ticks_us_default(n_tasks, -1);
        std::vector<long long> task_prios_default(n_tasks, 0);
        std::vector<float> task_ewr_default(n_tasks, 1);

#define M_GET_TASKS_VEC(dest, attr)                                            \
    (M_GET_ATTR(dest, attr),                                                   \
     exact_length<yaml_error_type::YAML_ERROR>(n_tasks, dest.size(), attr),    \
     (dest))

        M_GET_TASKS_VEC(task_names, "tasks_name");
        M_GET_TASKS_VEC(task_types, "tasks_type");
        M_GET_TASKS_VEC(task_wcets, "tasks_wcet");
        M_GET_TASKS_VEC(task_runtimes, "tasks_runtime");
        M_GET_TASKS_VEC(task_rel_deadlines, "tasks_rel_deadline");
        M_GET_TASKS_VEC(task_affinities, "tasks_affinity");

        M_GET_ATTR(adj_mat, "adjacency_matrix");

#define M_GET_ATTR_OPT(dest, attr, def_v)                                      \
    dest = get_attribute<decltype(dest), yaml_error_type::YAML_WARN>(          \
        input, attr, fname, def_v)

#define M_GET_TASKS_VEC_OPT(dest, attr, def_v)                                 \
    (M_GET_ATTR_OPT(dest, attr, def_v),                                        \
     exact_length<yaml_error_type::YAML_WARN>(n_tasks, dest.size(), attr),     \
     (dest))

        // Get attribute if available or use default vector.
        //
        // NOTE: if an attribute is read from the file, it MUST be the
        // right length! If the default value is used no issue will arise
        // because it is already the right length.
        M_GET_TASKS_VEC_OPT(task_matrix_size, "tasks_matrix_size",
                            task_matrix_size_default);
        M_GET_TASKS_VEC_OPT(task_omp_target, "tasks_omp_target",
                            task_omp_target_default);
        M_GET_TASKS_VEC_OPT(task_ticks_us, "tasks_ticks_per_us",
                            task_ticks_us_default);
        M_GET_TASKS_VEC_OPT(task_ewr, "tasks_expected_wcet_ratio",
                            task_ewr_default);

        M_GET_TASKS_VEC_OPT(task_prios, "tasks_prio", task_prios_default);

        // Check in both directions
        exact_length<yaml_error_type::YAML_ERROR>(n_tasks, adj_mat.size(),
                                                  "adjacency_matrix");
        for (const auto &line : adj_mat) {
            exact_length<yaml_error_type::YAML_ERROR>(n_tasks, line.size(),
                                                      "adjacency_matrix");
        }

        // FIXME: fred_ids
#if RTDAG_FRED_SUPPORT == ON
        std::vector<int> fred_ids;
        M_GET_TASKS_VEC(fred_ids, "fred_id");
#endif

        // Copy data back into array and matrix
        for (int i = 0; i < n_tasks; ++i) {
            tasks[i] = {
                .name = task_names[i],
                .type = task_types[i],
                .prio = task_prios[i],
                .wcet = task_wcets[i],
                .runtime = task_runtimes[i],
                .rel_deadline = task_rel_deadlines[i],
                .affinity = task_affinities[i],
                .matrix_size = task_matrix_size[i],
                .omp_target = task_omp_target[i],
                .ticks_per_us = task_ticks_us[i],
                .expected_wcet_ratio = task_ewr[i],

#if RTDAG_FRED_SUPPORT == ON
                .fred_id = fred_ids[i],
#endif
            };
        }

        for (int i = 0; i < n_tasks; ++i) {
            for (int j = 0; j < n_tasks; ++j) {
                adjacency_matrix[i][j] = adj_mat[i][j];
            }
        }

#undef M_GET_ATTR
#undef M_GET_TASKS_VEC
    }

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

    unsigned long get_tasks_prio(unsigned t) const override {
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
        return tasks[t].expected_wcet_ratio; // FIXME: this may be optional
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
