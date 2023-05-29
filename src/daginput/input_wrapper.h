#ifndef INPUT_WRAPPER_H_
#define INPUT_WRAPPER_H_

/*
This is a wrapper class to ease the integration of multiple input formats for
rt-dag.
*/
#include <iostream>

using namespace std;

class input_wrapper {
public:
    // not supported by older compilers
    //input_wrapper([[maybe_unused]] const char *fname_) {}
    input_wrapper(const char *fname_) {(void) fname_; }
    virtual ~input_wrapper() = default;

    void dump() {
        cout << "dag_name: " << get_dagset_name() << endl;
        cout << "n_tasks: " << get_n_tasks() << endl;
        cout << "n_edges: " << get_n_edges() << endl;
        cout << "n_cpus: " << get_n_cpus() << endl;
        cout << "max_out_edges: " << get_max_out_edges() << endl;
        cout << "max_in_edges: " << get_max_in_edges() << endl;
        cout << "repetitions: " << get_repetitions() << endl;
        cout << "period: " << get_period() << endl;
        cout << "deadline: " << get_deadline() << endl;
        cout << endl;
        const int ntasks = get_n_tasks();
        cout << "tasks: \n";
        for (int i = 0; i < ntasks; i++) {
            cout << " - " << get_tasks_name(i) << ", " << get_tasks_type(i)
                 << ", " << get_tasks_wcet(i) << ", "
                 << get_tasks_rel_deadline(i) << ", " << get_tasks_affinity(i)
                 << endl;
        }
    }

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
    virtual int get_fred_id(unsigned t) const = 0;
    virtual unsigned long get_tasks_runtime(unsigned t) const = 0;
    virtual unsigned long get_tasks_wcet(unsigned t) const = 0;
    virtual unsigned long get_tasks_rel_deadline(unsigned t) const = 0;
    virtual int get_tasks_affinity(unsigned t) const = 0;
    virtual unsigned get_adjacency_matrix(unsigned t1, unsigned t2) const = 0;
    virtual float get_expected_wcet_ratio() const = 0;
};

#endif // INPUT_WRAPPER_H_