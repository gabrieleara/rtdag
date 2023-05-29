
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

void exit_all([[maybe_unused]] int sigid){
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

int get_ticks_per_us(bool required);

int run_dag(string in_fname) {
  // uncomment this to get a random seed
  //unsigned seed = time(0);
  // or set manually a constant seed to repeat the same sequence
  unsigned seed = 123456;
  cout << "SEED: " << seed << endl;

  // read the dag configuration from the selected type of input
  std::unique_ptr<input_wrapper> inputs = std::make_unique<input_type>(in_fname.c_str());
  inputs->dump();
  TaskSet task_set(inputs);
  cout << "\nPrinting the input DAG: \n";
  task_set.print();

  // Check whether the environment contains the TICKS_PER_US variable
  int ret = get_ticks_per_us(true);
  if (ret) {
    return ret;
  }

  // create the directory where execution time are saved
  struct stat st; // This is C++, you cannot use {0} to initialize to zero an entire struct.
  memset(&st, 0, sizeof(struct stat));
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
  // "" is used only to avoid variadic macro warning
  LOG(INFO,"[main] all tasks were finished%s...\n"," ");

  return 0;
}

// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║                          Calibration and Testing                          ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

#ifdef USE_COMPILER_BARRIER
// It tells the compiler to not reorder instructions around it 
#define COMPILER_BARRIER() asm volatile("" ::: "memory")
#else
#define COMPILER_BARRIER()
#endif

int get_ticks_per_us(bool required) {
    if (ticks_per_us > 0) {
        return EXIT_SUCCESS;
    }

    char* TICKS_PER_US = getenv("TICKS_PER_US");

    auto &print_stream = (required) ? cerr : cout;
    auto kind = (required) ? "ERROR" : "WARN";

    if (TICKS_PER_US == nullptr) {
        print_stream << kind << ": TICKS_PER_US undefined!" << endl;
        return EXIT_FAILURE;
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

int test_calibration(uint64_t duration_us, uint64_t &time_difference) {
    int res = get_ticks_per_us(true);
    if (res)
        return res;

    COMPILER_BARRIER();

    auto time_before = micros();

    COMPILER_BARRIER();

    uint64_t retv = Count_Time_Ticks(duration_us);
    (void) (retv);

    COMPILER_BARRIER();

    auto time_after = micros();

    COMPILER_BARRIER();

    time_difference = time_after - time_before;
    cout << "Test duration: " << time_difference << " micros" << endl;

    return 0;
}

int test_calibration(uint64_t duration_us) {
    uint64_t time_difference_unused;
    return test_calibration(duration_us, time_difference_unused);
}

int calibrate(uint64_t duration_us) {
    int ret;
    uint64_t time_difference = 1;

    ret = get_ticks_per_us(false);
    if (ret) {
        // Set a value that is not so small in ticks_per_us
        ticks_per_us = 10;
    }

    cout << "About to calibrate for (roughly) "<< duration_us << " micros ..." << endl;

    // Will never return an error
    test_calibration(duration_us, time_difference);
    // fprintf(stderr, "DEBUG: %llu %llu %llu %llu\n", duration_us, time_difference, ticks_per_us, duration_us * ticks_per_us);

    using ticks_type = decltype(ticks_per_us);

    ticks_per_us = ticks_type(double(duration_us * ticks_per_us) / double(time_difference));

    cout << "Calibration successful, use: 'export TICKS_PER_US=" << ticks_per_us << "'" << endl;

    return EXIT_SUCCESS;
}

// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║                          Command Line Arguments                           ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

void usage(char *program_name){
    auto usage_format = R"STRING(Usage: %s [ OPTION %s]
Developed by ReTiS Laboratory, Scuola Sant'Anna, Pisa (2022).

Where OPTION can be (only) one of the following:
    -h, --help                  Display this helpful message
    -c USEC, --calibrate USEC   Run a calibration diagnostic for count_ticks
    -t USEC, --test USEC        Test calibration accuracy for count_ticks

If no OPTION is supplied, a DAG is run. The input mode for specifying the DAG
information is: %s.

The exception to the above rule is the following OPTION, which will instead
affect only runs that specify a DAG file:

    -e RATIO, --expected RATIO

The RATIO parameter is a float that shall be greater than 0 and less than or
equal to 1.0. It is used to compute the actual runtime of a task (in a
frequency-independent way) as a fraction of its WCET.
This OPTION must come BEFORE the DAG definition filename, if the input mode
compiled against this program requires one.

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

enum class command_action {
    HELP,
    RUN_DAG,
    CALIBRATE,
    TEST,
};

struct opts {
    command_action action = command_action::RUN_DAG;
    string in_fname = "";
    uint64_t duration_us = 0;
    float expected_wcet_ratio = 0;
    int exit_code = EXIT_SUCCESS;
};

struct nullopt_t {
    struct init {};
    nullopt_t(init) {}
};
const nullopt_t nullopt = nullopt_t(nullopt_t::init());

// Dummy version of std::optional for C++ version less than C++17. Does NOT
// handle all the things that std::optional does. Tested only on primitive
// types.
//
// Type CANNOT be a reference, but it is not tested in the code.
template <class Type>
class optional {
    bool valid;

    // NOTE: this element will 99% default initialized when the optional is
    // invalid! The solution would be to use an union (etc.etc.), but we don't
    // need it for this project. (One could argue we don't need this class but
    // whatever.)
    Type value;

public:
    optional() : valid(false) {}

    // Implicit conversion from any nullopt value to empty optional
    optional(nullopt_t) : valid(false) {}

    // Copy and move constructors and operators, necessary, otherwise it will
    // falsely use one of the other constructors when copying/moving
    optional<Type>(const optional<Type> &rhs) = default;
    optional<Type>(optional<Type> &&rhs) = default;
    optional<Type>& operator=(const optional<Type> &rhs) = default;
    optional<Type>& operator=(optional<Type> &&rhs) = default;

    // Copy from Type
    explicit constexpr optional(const Type &v) : valid(true), value(v) {}

    // Move from Type
    explicit constexpr optional(Type &&v) : valid(true), value(std::move(v)) {}

    // Construction from the arguments of the underlying type
    template <class... Args>
    explicit optional(Args &&...args) :
        valid(true), value(std::forward<Args>(args)...) {}

    // Construction from initializer list (+ optional arguments)
    template <class U, class... Args>
    explicit optional(std::initializer_list<U> il, Args &&...args) :
        valid(true), value(il, std::forward<Args>(args)...) {}

    // Destructor
    ~optional() {
        if (valid) {
            value.Type::~Type();
        }
    }

    // Conversion to bool returns whether the optional is empty
    explicit operator bool() const {
        return valid;
    }

    // Operator * returns the original value
    Type operator*() {
        if (!valid)
            throw std::runtime_error("Attempt accessing an empty optional!");
        return value;
    }

    Type* operator->() {
        if (!valid)
            throw std::runtime_error("Attempt accessing an empty optional!");
        return &value;
    }
};

template<class ReturnType>
optional<ReturnType> parse_argument_from_string(const char *str) {
    auto mstring = std::string(str);
    auto mstream = std::istringstream(mstring);

    ReturnType rt;
    mstream >> rt;

    return mstream ? optional<ReturnType>(rt) : nullopt;
}

opts parse_args(int argc, char *argv[]) {
    opts program_options;
    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"calibrate", required_argument, 0, 'c'},
            {"test", required_argument, 0, 't'},
            {"expected", required_argument, 0, 'e'},
            {0, 0, 0, 0}};

        int c = getopt_long(argc, argv, "hc:t:e:", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch(c) {
            case 0:
                // No long option has no corresponding short option
                fprintf(stderr, "Error: ?? getopt returned character code 0%o ??\n", c);
                program_options.exit_code = EXIT_FAILURE;
                assert(false);
                break;
            case 'h':
                program_options.action = command_action::HELP;
                goto end;
            case 'c':
            case 't': {
                auto duration_valid = parse_argument_from_string<uint64_t>(optarg);
                if (!duration_valid) {
                    goto duration_error;
                }
                program_options.duration_us = *duration_valid;
                program_options.action = c == 'c' ? command_action::CALIBRATE : command_action::TEST;

                if (program_options.duration_us > 0.0) {
                    goto end;
                }

            duration_error:
                fprintf(stderr, "Invalid argument to option: %s\n", optarg);
                program_options.action = command_action::HELP;
                program_options.exit_code = EXIT_FAILURE;
                goto end;
            }
            case 'e': {
                auto expected_valid = parse_argument_from_string<float>(optarg);
                if (!expected_valid) {
                    goto expected_error;
                }

                program_options.expected_wcet_ratio = *expected_valid;
                if (program_options.expected_wcet_ratio < 0.0 ||
                    program_options.expected_wcet_ratio > 1.0) {
                    goto expected_error;
                }

                break;

            expected_error:
                fprintf(stderr, "Invalid argument to option: %s\n", optarg);
                program_options.action = command_action::HELP;
                program_options.exit_code = EXIT_FAILURE;
                goto end;
            }
            case '?':
                // fprintf(stderr, "Error: ?? getopt returned character code
                // 0%o ??\n", c);
                program_options.action = command_action::HELP;
                program_options.exit_code = EXIT_FAILURE;
                goto end;
            default:
                fprintf(stderr, "Error: ?? getopt returned character code 0%o ??\n", c);
                program_options.exit_code = EXIT_FAILURE;
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
        program_options.action = command_action::HELP;
        program_options.exit_code = EXIT_FAILURE;
        goto end;
    }

#if INPUT_TYPE != 0
    if (program_options.in_fname.length() < 1) {
        fprintf(stderr, "Error: too few arguments supplied!\n");
        program_options.action = command_action::HELP;
        program_options.exit_code = EXIT_FAILURE;
        goto end;
    }
#endif

end:
    return program_options;
}

// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║                                   Main                                    ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

float expected_wcet_ratio_override = 0.0;

int main(int argc, char *argv[]) {
    // signal(SIGKILL,exit_all);
    // signal(SIGSEGV,exit_all);
    // signal(SIGINT,exit_all);

    // auto functor = functor_factory("OpenCL");
    // if (functor == nullptr)
    //     return EXIT_FAILURE;

    auto program_options = parse_args(argc, argv);
    expected_wcet_ratio_override = program_options.expected_wcet_ratio;

    switch (program_options.action) {
    case command_action::HELP:
        usage(argv[0]);
        return program_options.exit_code;

    case command_action::CALIBRATE:
        return calibrate(program_options.duration_us);

    case command_action::TEST:
        return test_calibration(program_options.duration_us);

    case command_action::RUN_DAG:
        if (program_options.exit_code != EXIT_SUCCESS) {
            return program_options.exit_code;
        }

        return run_dag(program_options.in_fname);
    }

    assert(false);
    cerr << "There's an error in the implementation!" << endl;
    return EXIT_FAILURE;
}
