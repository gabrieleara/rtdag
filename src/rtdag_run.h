#pragma once
#ifndef RTDAG_RUN_H
#define RTDAG_RUN_H

// the dag definition is here
#include "task_set.h"
#include "input.h"

using namespace std;

////////////////////////////
// globals used by the tasks
////////////////////////////
// check the end-to-end DAG deadline
// the start task writes and the final task reads from it to
// unsigned long dag_start_time;
vector<int> pid_list;

void exit_all([[maybe_unused]] int sigid) {
#if TASK_IMPL == TASK_IMPL_THREAD
    printf("Killing all threads\n");
    // TODO: how to kill the threads without access to the thread list ?
#else
    printf("Killing all tasks\n");
    unsigned i, ret;
    for (i = 0; i < pid_list.size(); ++i) {
        ret = kill(pid_list[i], SIGKILL);
        assert(ret == 0);
    }
#endif
    printf("Exting\n");
    exit(0);
}

int get_ticks_per_us(bool required);

int run_dag(string in_fname) {
    // uncomment this to get a random seed
    // unsigned seed = time(0);
    // or set manually a constant seed to repeat the same sequence
    unsigned seed = 123456;
    cout << "SEED: " << seed << endl;

    // read the dag configuration from the selected type of input
    std::unique_ptr<input_base> inputs =
        std::make_unique<input_type>(in_fname.c_str());
    dump(*inputs);
    TaskSet task_set(inputs);
    cout << "\nPrinting the input DAG: \n";
    task_set.print();

    // Check whether the environment contains the TICKS_PER_US variable
    int ret = get_ticks_per_us(true);
    if (ret) {
        return ret;
    }

    // create the directory where execution time are saved
    struct stat st; // This is C++, you cannot use {0} to initialize to zero an
                    // entire struct.
    memset(&st, 0, sizeof(struct stat));
    if (stat(task_set.get_dagset_name(), &st) == -1) {
        // permisions required in order to allow using rsync since rt-dag is run
        // as root in the target computer
        int rv = mkdir(task_set.get_dagset_name(), 0777);
        if (rv != 0) {
            perror("ERROR creating directory");
            exit(1);
        }
    }

    // pass pid_list such that tasks can be killed with CTRL+C
    task_set.launch_tasks(&pid_list, seed);
    // "" is used only to avoid variadic macro warning
    LOG(INFO, "[main] all tasks were finished%s...\n", " ");

    return 0;
}
#endif // RTDAG_RUN_H
