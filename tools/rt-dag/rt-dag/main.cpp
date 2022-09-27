
/*
 * Description: This application tests how to build a taskset described as a DAG using threads and a thread-safe queue for communication.
 *
 * Example DAG:
 *     n1
 *    /   \
 * n0      n3
 *    \   /
 *     n2
 *
 * n3 can only be unlocked when all its antecessors n1 and n2 finished.
 *
 *
 * Assumptions:
 *  - Each task is implemented w a thread or process. Check TASK_IMPL define;
 *  - Each edge is implemented with a thread-safe queue w size limite and blocking push. Syncronous blocking communication mode.
 *  - The DAG period is lower than the DAG WCET. This way, each shared variable it's garanteed that the next element will be received only after the current one is consumed
 *  - The 'task duration' accounts for the task execution plus msgs sent. It does not account waiting time for incomming data
 *
 * Author:
 *  Alexandre Amory (June 2022), ReTiS Lab, Scuola Sant'Anna, Pisa, Italy.
 *
 * Compilation:
 *  $> g++  main.cpp ../../../common/src/periodic_task.c ../../../common/src/time_aux.c -I ../../../common/include/ -I blocking_queue -g3 -o dag-launcher-thread -lrt -lpthread
 *
 * Usage Example:
 *  $> ./dag-launcher-thread
 */

#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <string>
#include <sstream>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <sys/types.h> // to create the directory
#include <sys/stat.h>

#include <unistd.h> // getpid

#include <getopt.h>

#include <log.h>
#include "input_wrapper.h"

// select the input type
#if INPUT_TYPE == 0
    #include "input_header.h"
    using input_type = input_header;
    #define INPUT_TYPE_NAME "header"
    #define INPUT_TYPE_NAME_CAPS "HEADER"
#else
    #include "input_yaml.h"
    using input_type = input_yaml;
    #define INPUT_TYPE_NAME "yaml"
    #define INPUT_TYPE_NAME_CAPS "YAML"
#endif

// the dag definition is here
#include "task_set.h"

using namespace std;

////////////////////////////
// globals used by the tasks
////////////////////////////
// check the end-to-end DAG deadline
// the start task writes and the final task reads from it to
// unsigned long dag_start_time;
vector<int> pid_list;

void exit_all(int sigid){
#if TASK_IMPL == 0
    printf("Killing all threads\n");
    // TODO: how to kill the threads without access to the thread list ?
#else
    printf("Killing all tasks\n");
    unsigned i,ret;
    for(i=0;i<pid_list.size();++i){
        ret = kill(pid_list[i],SIGKILL);
        assert(ret==0);
    }
#endif
    printf("Exting\n");
    exit(0);
}

void usage(char *program_name){
    auto usage_format = R"STRING(Usage: %s [ -h | --help | -p | --profile | -t USEC | --test-profile USEC %s]
Developed by ReTiS Laboratory, Scuola Sant'Anna, Pisa (2022).

Input mode: %s

)STRING";
    printf(usage_format, program_name,
    #if INPUT_TYPE != 0
        "| <INPUT_" INPUT_TYPE_NAME_CAPS "_FILE> "
    #else
        ""
    #endif
    , INPUT_TYPE_NAME
    );
}

struct opts {
    string in_fname = "";
    bool profile = false;
    uint64_t test_duration = 0;
    bool help = false;
    int exit_with_error = EXIT_SUCCESS;
};

struct test_duration_result {
    bool valid = true;
    uint64_t duration;
};

test_duration_result parse_test_duration(const char* duration) {
    auto mstring = std::string(duration);
    auto mstream = std::istringstream(mstring);

    test_duration_result res;
    mstream >> res.duration;
    res.valid = bool(mstream);
    return res;
}

opts parse_args(int argc, char *argv[]) {
    opts program_options;
    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h' },
            {"profile", no_argument, 0, 'p' },
            {"test-profile", required_argument, 0, 't' },
            {0, 0, 0, 0}
        };

        char c = getopt_long(argc, argv, "hpt:", long_options, &option_index);
        if (c == -1) {
            break;
        }

        test_duration_result duration_res;

        switch(c) {
            case 0:
                // No long option has no corresponding short option
                fprintf(stderr, "Error: ?? getopt returned character code 0%o ??\n", c);
                program_options.exit_with_error = EXIT_FAILURE;
                assert(false);
                break;
            case 'h':
                program_options.help = true;
                goto end;
            case 'p':
                program_options.profile = true;
                goto end;
            case 't':
                duration_res = parse_test_duration(optarg);
                if (!duration_res.valid) {
                    fprintf(stderr, "Invalid argument to option: %s\n", optarg);
                    program_options.help = true;
                    program_options.exit_with_error = EXIT_FAILURE;
                    goto end;
                }

                program_options.test_duration = duration_res.duration;
                goto end;
            case '?':
                // fprintf(stderr, "Error: ?? getopt returned character code 0%o ??\n", c);
                program_options.help = true;
                program_options.exit_with_error = EXIT_FAILURE;
                goto end;
            default:
                fprintf(stderr, "Error: ?? getopt returned character code 0%o ??\n", c);
                program_options.exit_with_error = EXIT_FAILURE;
                goto end;
        }
    }

  // this input format does not have an input file format.
  // INPUT_TYPE != 0  means this is not the input_header mode, which does not have input files
#if INPUT_TYPE != 0
    if (optind < argc) {
        program_options.in_fname = argv[optind++];
    }
#endif

    if (optind < argc) {
        fprintf(stderr, "Error: too many arguments supplied!\n");
        program_options.help = true;
        program_options.exit_with_error = EXIT_FAILURE;
        goto end;
    }

#if INPUT_TYPE != 0
    if (program_options.in_fname.length() < 1) {
        fprintf(stderr, "Error: too few arguments supplied!\n");
        program_options.help = true;
        program_options.exit_with_error = EXIT_FAILURE;
        goto end;
    }
#endif

end:
    return program_options;
}

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>

#include "time_aux.h"

constexpr uint64_t ticks_profile = 1'000'000'000;
#ifdef USE_COMPILER_BARRIER
#define COMPILER_BARRIER() asm volatile("" ::: "memory")
#else
#define COMPILER_BARRIER()
#endif

extern uint64_t ticks_per_us;

int get_ticks_per_us();

struct test_profile_ret {
    int success = 0;
    uint64_t time_difference;
};

test_profile_ret do_test_profile(uint64_t duration_us) {
    int res = get_ticks_per_us();
    if (res) {
        return {
            res,
        };
    }

    COMPILER_BARRIER();

    auto time_before = micros();

    COMPILER_BARRIER();

    uint64_t retv = do_work_for_us_using_ticks(duration_us);
    (void) (retv);

    COMPILER_BARRIER();

    auto time_after = micros();

    COMPILER_BARRIER();

    cout << "Time difference (micros): " << (time_after - time_before) << endl;

    return {
        0,
        (time_after - time_before),
    };
}

int do_profile() {
    // Set a value that is not so small in ticks_per_us
    ticks_per_us = 300;

    uint64_t duration_us = ticks_profile / ticks_per_us;

    cout << "About to profile..." << endl;

    auto res = do_test_profile(duration_us);
    ticks_per_us = floor(double(duration_us * ticks_per_us) / double(res.time_difference));

    auto res_string = R"(Profile successful!
Please run this command before running this program at the same frequency next time:
export TICKS_PER_US='%lu'
)";
    printf(res_string, ticks_per_us);

    return EXIT_SUCCESS;
}

int get_ticks_per_us() {
    if (ticks_per_us > 0)
        return EXIT_SUCCESS;

    char* TICKS_PER_US = getenv("TICKS_PER_US");

    if (TICKS_PER_US == nullptr) {
        cout << "WARN: TICKS_PER_US undefined!" << endl;
        int res = do_profile();
        if (res) {
            return res;
        }
    } else {
        auto mstring = std::string(TICKS_PER_US);
        auto mstream = std::istringstream(mstring);

        mstream >> ticks_per_us;
        if (!mstream) {
            cerr << "Error! could not parse environment variable" << endl;
        }
    }

    cout << "Using the following value for time accounting: " << ticks_per_us << endl;

    return EXIT_SUCCESS;

}

int main(int argc, char* argv[]) {

  // signal(SIGKILL,exit_all);
  // signal(SIGSEGV,exit_all);
  // signal(SIGINT,exit_all);

    auto program_options = parse_args(argc, argv);

    if (program_options.help) {
        usage(argv[0]);
        return program_options.exit_with_error;
    }

    if (program_options.exit_with_error != EXIT_SUCCESS) {
        return program_options.exit_with_error;
    }

    if (program_options.profile) {
        return do_profile();
    }

    if (program_options.test_duration) {
        return do_test_profile(program_options.test_duration).success;
    }

  string in_fname = "";
#if INPUT_TYPE != 0
    in_fname = program_options.in_fname;
#endif

  // uncomment this to get a random seed
  //unsigned seed = time(0);
  // or set manually a constant seed to repeat the same sequence
  unsigned seed = 123456;
  cout << "SEED: " << seed << endl;

  // read the dag configuration from the selected type of input
  std::unique_ptr<input_wrapper> inputs = std::make_unique<input_type>(in_fname.c_str());
  inputs->dump();
  // build the TaskSet class of data from dag.h
  TaskSet task_set(inputs);
  task_set.print();

  // Check whether the environment contains the TICKS_PER_US variable
  get_ticks_per_us();

  // create the directory where execution time are saved
  struct stat st = {0};
  if (stat(task_set.get_dagset_name(), &st) == -1) {
    // permisions required in order to allow using rsync since rt-dag is run as root in the target computer
    int rv = mkdir(task_set.get_dagset_name(), 0777);
    if (rv != 0) {
        perror("ERROR creating directory");
        exit(1);
    }
  }

  // pass pid_list such that tasks can be killed with CTRL+C
  task_set.launch_tasks(&pid_list,seed);

  LOG(INFO,"[main] all tasks were finished ...\n");

  return 0;
}
