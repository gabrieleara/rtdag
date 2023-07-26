#include "input/yaml.h"

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

// ------------------------- Member Functions -------------------------- //

#define GET_ATTR_REQ(dest, attr)                                               \
    (dest = get_attribute<decltype(dest), yaml_error_type::YAML_ERROR>(        \
         input, attr, fname))

#define GET_ATTR_OPT(dest, attr, def_v)                                        \
    dest = get_attribute<decltype(dest), yaml_error_type::YAML_WARN>(          \
        input, attr, fname, def_v)

#define GET_VECT_REQ(dest, attr)                                               \
    (GET_ATTR_REQ(dest, attr),                                                 \
     exact_length<yaml_error_type::YAML_ERROR>(n_tasks, dest.size(), attr),    \
     (dest))

#define GET_VECT_OPT(dest, attr, def_v)                                        \
    (GET_ATTR_OPT(dest, attr, def_v),                                          \
     exact_length<yaml_error_type::YAML_WARN>(n_tasks, dest.size(), attr),     \
     (dest))

input_yaml::input_yaml(const char *fname) : input_base() {
    YAML::Node input = read_yaml_file(fname);

    GET_ATTR_REQ(repetitions, "repetitions");
    GET_ATTR_REQ(hyperperiod, "hyperperiod");
    GET_ATTR_REQ(cpu_freqs, "cpus_freq");

    int n_cpus;
    GET_ATTR_REQ(n_cpus, "n_cpus");
    exact_length<yaml_error_type::YAML_WARN>(n_cpus, cpu_freqs.size(),
                                             "cpus_freq");

    GET_ATTR_REQ(dag_name, "dag_name");
    GET_ATTR_REQ(n_edges, "n_edges");
    GET_ATTR_REQ(max_out_edges, "max_out_edges");
    GET_ATTR_REQ(max_in_edges, "max_in_edges");
    GET_ATTR_REQ(max_msg_len, "max_msg_len");

    GET_ATTR_REQ(dag_period, "dag_period");
    GET_ATTR_REQ(dag_deadline, "dag_deadline");

    GET_ATTR_REQ(n_tasks, "n_tasks");
    if (n_tasks < 1) {
        std::fprintf(stderr, "ERROR: negative value in 'n_tasks'\n");
        std::exit(EXIT_FAILURE);
    }

    if (((unsigned long)(n_tasks)) > MAX_N_TASKS) {
        std::fprintf(stderr,
                     "ERROR: 'n_tasks' greater than maximum allowed: %d > %lu\n",
                     n_tasks, MAX_N_TASKS);
        std::fprintf(stderr, "Please, recompile rtdag to increase the "
                             "maximum allowed number.\n");
        std::exit(EXIT_FAILURE);
    }

    std::vector<std::string> task_names;
    std::vector<std::string> task_types;
    std::vector<int> task_prios;
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
    std::vector<int> task_prios_default(n_tasks, 0);
    std::vector<float> task_ewr_default(n_tasks, 1);

    GET_VECT_REQ(task_names, "tasks_name");
    GET_VECT_REQ(task_types, "tasks_type");
    GET_VECT_REQ(task_wcets, "tasks_wcet");
    GET_VECT_REQ(task_runtimes, "tasks_runtime");
    GET_VECT_REQ(task_rel_deadlines, "tasks_rel_deadline");
    GET_VECT_REQ(task_affinities, "tasks_affinity");

    GET_ATTR_REQ(adj_mat, "adjacency_matrix");

    // Get attribute if available or use default vector.
    //
    // NOTE: if an attribute is read from the file, it MUST be the
    // right length! If the default value is used no issue will arise
    // because it is already the right length.
    GET_VECT_OPT(task_matrix_size, "tasks_matrix_size",
                 task_matrix_size_default);
    GET_VECT_OPT(task_omp_target, "tasks_omp_target", task_omp_target_default);
    GET_VECT_OPT(task_ticks_us, "tasks_ticks_per_us", task_ticks_us_default);
    GET_VECT_OPT(task_ewr, "tasks_expected_wcet_ratio", task_ewr_default);

    GET_VECT_OPT(task_prios, "tasks_prio", task_prios_default);

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
    GET_VECT_REQ(fred_ids, "fred_id");
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

#undef GET_ATTR_REQ
#undef GET_VECT_REQ
}
