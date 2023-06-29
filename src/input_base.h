#ifndef RTDAG_INPUT_BASE_H
#define RTDAG_INPUT_BASE_H

#include <cstdio>
#include <type_traits>

class input_base {
public:
    // No need to provide a constructor that will not be used, we will check it
    // later! input_base([[maybe_unused]] const char *fname_) {}
    virtual ~input_base() = default;

    virtual const char *get_dagset_name() const = 0;
    virtual unsigned get_n_tasks() const = 0;
    virtual unsigned get_n_edges() const = 0;
    virtual unsigned get_n_cpus() const = 0;
    virtual unsigned get_max_out_edges() const = 0;
    virtual unsigned get_max_in_edges() const = 0;
    virtual unsigned get_msg_len() const = 0;
    virtual unsigned get_repetitions() const = 0;
    virtual unsigned long get_period() const = 0;
    virtual unsigned long get_deadline() const = 0;
    virtual unsigned long get_hyperperiod() const = 0;
    virtual const char *get_tasks_name(unsigned t) const = 0;
    virtual const char *get_tasks_type(unsigned t) const = 0;
#if RTDAG_FRED_SUPPORT == ON
    virtual int get_fred_id(unsigned t) const = 0;
#endif
    virtual unsigned long get_tasks_prio(unsigned t) const = 0;
    virtual unsigned long get_tasks_runtime(unsigned t) const = 0;
    virtual unsigned long get_tasks_wcet(unsigned t) const = 0;
    virtual unsigned long get_tasks_rel_deadline(unsigned t) const = 0;
    virtual int get_tasks_affinity(unsigned t) const = 0;
    virtual unsigned get_adjacency_matrix(unsigned t1, unsigned t2) const = 0;
    virtual float get_tasks_expected_wcet_ratio(unsigned t) const = 0;

    virtual unsigned int get_matrix_size(unsigned t) const = 0;
    virtual unsigned int get_omp_target(unsigned t) const = 0;
    virtual float get_ticks_per_us(unsigned t) const = 0;
};

static inline void dump(const input_base &in) {
    // FIXME: the dump is probably not complete
    std::printf("dag_name:      %s\n", in.get_dagset_name());
    std::printf("n_tasks:       %u\n", in.get_n_tasks());
    std::printf("n_edges:       %u\n", in.get_n_edges());
    std::printf("n_cpus:        %u\n", in.get_n_cpus());
    std::printf("max_out_edges: %u\n", in.get_max_out_edges());
    std::printf("max_in_edges:  %u\n", in.get_max_in_edges());
    std::printf("repetitions:   %u\n", in.get_repetitions());
    std::printf("period:        %lu\n", in.get_period());
    std::printf("deadline:      %lu\n", in.get_deadline());
    std::printf("\n");
    std::printf("tasks:\n");
    for (int i = 0, n_tasks = in.get_n_tasks(); i < n_tasks; ++i) {
        std::printf(" - %s, %s, %ld, %ld, %d\n", in.get_tasks_name(i),
                    in.get_tasks_type(i), in.get_tasks_wcet(i),
                    in.get_tasks_rel_deadline(i), in.get_tasks_affinity(i));
    }
}

// ------------------- C++20 Concepts ------------------- //

template <typename T>
concept derives_from_input_base = std::is_base_of<input_base, T>::value;

// Can be constructed using a const char*
template <typename T>
concept filename_constructible = std::is_constructible<T, const char *>::value;

template <typename T>
concept dag_input_class =
    derives_from_input_base<T> && filename_constructible<T>;

#endif // RTDAG_INPUT_BASE_H
