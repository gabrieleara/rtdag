#include "newstuff/taskset.h"
#include <pthread.h>

static inline std::vector<int> output_tasks(const input_base &input,
                                            int task_id) {
    const int ntasks = input.get_n_tasks();
    std::vector<int> v;
    for (int col = 0; col < ntasks; ++col) {
        if (input.get_adjacency_matrix(task_id, col) != 0) {
            v.push_back(col);
        }
    }
    return v;
}

static inline std::vector<int> input_tasks(const input_base &input,
                                           int task_id) {
    const int ntasks = input.get_n_tasks();
    std::vector<int> v;
    for (int row = 0; row < ntasks; ++row) {
        if (input.get_adjacency_matrix(row, task_id) != 0) {
            v.push_back(row);
        }
    }
    return v;
}

static inline int howmany_inputs(const input_base &input, int task_id) {
    const int ntasks = input.get_n_tasks();
    int count = 0;
    for (int row = 0; row < ntasks; ++row) {
        if (input.get_adjacency_matrix(row, task_id) != 0) {
            count++;
        }
    }
    return count;
}

static inline s64 num_activations(std::chrono::microseconds hyperperiod,
                                  std::chrono::microseconds period,
                                  s64 repetitions) {
    return hyperperiod / period * repetitions;
}

static inline void
task_single_check(const std::vector<std::unique_ptr<Task>> &tasks,
                  const auto predicate, const std::string &which) {
    auto predicate_on_pointer =
        [predicate](const std::unique_ptr<Task> &task_ptr) {
            return predicate(*task_ptr);
        };

    auto begin = std::begin(tasks);
    auto end = std::end(tasks);

    const auto task_iter = std::find_if(begin, end, predicate_on_pointer);
    if (task_iter == end) {
        LOG(ERROR, "Could not find the %s task!\n", which.c_str());
        exit(EXIT_FAILURE);
    }

    // NOTE: only ONE task must match the predicate!
    const auto none_check =
        std::find_if(task_iter + 1, end, predicate_on_pointer);
    if (none_check != end) {
        LOG(ERROR, "Found multiple %s tasks!\n", which.c_str());
        exit(EXIT_FAILURE);
    }
}

DagTaskset::DagTaskset(const input_base &input) :
    dag(input.get_dagset_name(), std::chrono::microseconds(input.get_period()),
        std::chrono::microseconds(input.get_deadline()),
        num_activations(std::chrono::microseconds(input.get_hyperperiod()),
                        std::chrono::microseconds(input.get_period()),
                        input.get_repetitions()),
        input.get_n_tasks()) {
    int ntasks = input.get_n_tasks();

    // Create the in_queues for each task
    for (int task_id = 0; task_id < ntasks; ++task_id) {
        int inputs_count = howmany_inputs(input, task_id);
        if (inputs_count < 1) {
            inputs_count = 1; // It will not be used, but
        }
        dag.in_queues.emplace_back(std::make_unique<MultiQueue>(inputs_count));
    }

    // All the in_queues are in place, now we can create the edges
    for (int receiver = 0; receiver < ntasks; ++receiver) {
        int push_idx = 0;
        for (int sender = 0; sender < ntasks; ++sender) {
            int msg_size = input.get_adjacency_matrix(sender, receiver);
            if (msg_size < 1) {
                continue;
            }

            // There is an edge from sender to receiver of msg_size bytes
            dag.edges.emplace_back(*dag.in_queues[receiver], sender, receiver,
                                   push_idx, msg_size);

            push_idx++;
        }
    }

    // Finally, now that we have all the data, we can create the tasks
    for (int i = 0; i < ntasks; ++i) {
        const std::string name = input.get_tasks_name(i);
        const int cpu = input.get_tasks_affinity(i);
        sched_info sched_info{
            input.get_tasks_prio(i),
            std::chrono::microseconds(input.get_tasks_runtime(i)),
            std::chrono::microseconds(input.get_tasks_rel_deadline(i)),
            dag.period};

        std::vector<Edge *> in_edges;
        std::vector<Edge *> out_edges;

        for (Edge &edge : dag.edges) {
            if (edge.from == i) {
                out_edges.emplace_back(&edge);
            } else if (edge.to == i) {
                in_edges.emplace_back(&edge);
            }
        }

        std::string task_type = input.get_tasks_type(i);

        if (task_type == "cpu") {
            tasks.emplace_back(std::make_unique<CPUTask>(
                dag, name, task_type, sched_info, cpu, in_edges, out_edges,
                std::chrono::microseconds(input.get_tasks_wcet(i)),
                input.get_tasks_expected_wcet_ratio(i),
                input.get_ticks_per_us(i), input.get_matrix_size(i),
                input.get_omp_target(i)));
        }
#if RTDAG_OMP_SUPPORT == ON
        else if (task_type == "omp") {
            tasks.emplace_back(std::make_unique<OMPTask>(
                dag, name, task_type, sched_info, cpu, in_edges, out_edges,
                std::chrono::microseconds(input.get_tasks_wcet(i)),
                input.get_tasks_expected_wcet_ratio(i),
                input.get_ticks_per_us(i), input.get_matrix_size(i),
                input.get_omp_target(i)));
        }
#endif
        // TODO: FRED
        else {
            LOG(ERROR, "Unsupported task type %s\n.", task_type.c_str());
        }
    }

    const auto is_originator = [](const Task &task) {
        return task.is_originator();
    };

    const auto is_sink = [](const Task &task) { return task.is_originator(); };

    task_single_check(tasks, is_originator, "originator");
    task_single_check(tasks, is_sink, "sink");
}

void DagTaskset::print(std::ostream &os) {
    for (const auto &task_ptr : tasks) {
        task_ptr->print(os);
    }
    os.flush();
}

void DagTaskset::launch(std::vector<int> &pids, unsigned seed) {
    std::vector<std::thread> threads;

    for (const auto &task_ptr : tasks) {
        threads.emplace_back(task_ptr->start(seed));

        // FIXME: save the tid once the thread starts in a
        // shared box and retrieve it from here!

        (void)pids;

        // pthread_t handle = threads.back().native_handle();
        // pthread_id_np_t tid;
        // pthread_getunique_np(&self, &tid);

        // pids.push_back(tid);
    }

    for (auto & thread : threads) {
        thread.join();
    }
}
