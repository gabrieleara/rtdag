#ifndef RTDAG_COMMAND_H
#define RTDAG_COMMAND_H

#include "input.h"
#include <getopt.h>

#include <cassert>
#include <optional>

#include "rtgauss.h"

#define TASK_TYPES_CPU "cpu "
#if RTDAG_OMP_SUPPORT == ON
#define TASK_TYPES_OMP "omp "
#define HELP_OMP_TARGET                                                        \
    "-T OMP_TARGET[=0]           The OpenMP target to run the task in (if "    \
    "'omp' selected)"
#else
#define TASK_TYPES_OMP ""
#define HELP_OMP_TARGET ""
#endif

#define SUPPORTED_TASK_TYPES TASK_TYPES_CPU TASK_TYPES_OMP

// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║                          Command Line Arguments                           ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

void usage(char *program_name) {
    auto usage_format = R"STRING(Usage: %s [ OPTION %s]
Developed by ReTiS Laboratory, Scuola Sant'Anna, Pisa (2022).

Where OPTION can be (only) one of the following:
    -h, --help                  Display this helpful message
    -c USEC, --calibrate USEC   Run a calibration diagnostic for count_ticks
    -t USEC, --test USEC        Test calibration accuracy for count_ticks

The following options are used in combination with -c or -t, ignored otherwise:
    -C TASK_TYPE[=cpu]          Accepts a task type that supports
                                calibration to do the test
    -M MATRIX_SIZE[=4]          The size of the matrix used in calibration
                                tests
    %s


Accepted task types: %s

So if you want for example to calibrate a 'cpu' task multiplying two 10x10
matrices you can do it by passing -c USEC -C cpu -M 10

If no OPTION is supplied, a DAG is run. The input mode for specifying the DAG
information is: %s.

)STRING";

    if constexpr (input_type::has_input_file) {
        printf(usage_format, program_name,
               "| <INPUT_" INPUT_TYPE_NAME_CAPS "_FILE> ", HELP_OMP_TARGET,
               SUPPORTED_TASK_TYPES, INPUT_TYPE_NAME);
    } else {
        printf(usage_format, program_name, "", HELP_OMP_TARGET,
               SUPPORTED_TASK_TYPES, INPUT_TYPE_NAME);
    }
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
    rtgauss_type rtg_type = RTGAUSS_CPU;
    int rtg_target = 0;
    int rtg_msize = 4;
    int exit_code = EXIT_SUCCESS;
};

template <class ReturnType>
optional<ReturnType> parse_argument_from_string(const char *str) {
    auto mstring = std::string(str);
    auto mstream = std::istringstream(mstring);

    ReturnType rt;
    mstream >> rt;

    return mstream ? optional<ReturnType>(rt) : nullopt;
}

template <>
optional<rtgauss_type> parse_argument_from_string(const char *str) {
    auto mstring = std::string(str);

    if (mstring == "cpu") {
        return optional<rtgauss_type>(RTGAUSS_CPU);
    }
#if RTDAG_OMP_SUPPORT == ON
    if (mstring == "omp") {
        return optional<rtgauss_type>(RTGAUSS_OMP);
    }
#endif

    return nullopt;
}

opts parse_args(int argc, char *argv[]) {
    opts program_options;
    char the_option = ' ';
    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"calibrate", required_argument, 0, 'c'},
            {"test", required_argument, 0, 't'},
            {0, 0, 0, 0}};

        int c = getopt_long(argc, argv,
                            "hc:t:C:M:"
#if RTDAG_OMP_SUPPORT == ON
                            "T:"
#endif
                            ,
                            long_options, &option_index);
        if (c == -1) {
            break;
        }

        the_option = c;

        switch (c) {
        case 0:
            // No long option has no corresponding short option
            fprintf(stderr, "Error: ?? getopt returned character code 0%o ??\n",
                    c);
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
                goto arg_error;
            }
            program_options.duration_us = *duration_valid;
            program_options.action =
                c == 'c' ? command_action::CALIBRATE : command_action::TEST;
            break;
        }
        case 'C': {
            auto type_valid = parse_argument_from_string<rtgauss_type>(optarg);
            if (!type_valid) {
                goto arg_error;
            }

            program_options.rtg_type = *type_valid;
            break;
        }
        case 'M': {
            auto msize = parse_argument_from_string<int>(optarg);
            if (!msize) {
                goto arg_error;
            }

            program_options.rtg_msize = *msize;
            if (program_options.rtg_msize <= 0) {
                goto arg_error;
            }
            break;
        }
        case 'T': {
            auto target = parse_argument_from_string<int>(optarg);
            if (!target) {
                goto arg_error;
            }

            program_options.rtg_target = *target;
            if (program_options.rtg_target < 0) {
                goto arg_error;
            }
            break;
        }
        case '?':
            // fprintf(stderr, "Error: ?? getopt returned character code
            // 0%o ??\n", c);
            program_options.action = command_action::HELP;
            program_options.exit_code = EXIT_FAILURE;
            goto end;
        default:
            fprintf(stderr, "Error: ?? getopt returned character code 0%o ??\n",
                    c);
            program_options.exit_code = EXIT_FAILURE;
            goto end;
        }
    }

    if (program_options.action != command_action::RUN_DAG) {
        goto end;
    }

    // static check whether this input format does have an input file or
    // not
    if constexpr (input_type::has_input_file) {
        if (optind < argc) {
            program_options.in_fname = argv[optind++];
        }
    }

    if (optind < argc) {
        fprintf(stderr, "Error: too many arguments supplied!\n");
        program_options.action = command_action::HELP;
        program_options.exit_code = EXIT_FAILURE;
        goto end;
    }

    // static check whether this input format does have an input file or
    // not
    if constexpr (input_type::has_input_file) {
        if (program_options.in_fname.length() < 1) {
            fprintf(stderr, "Error: too few arguments supplied!\n");
            program_options.action = command_action::HELP;
            program_options.exit_code = EXIT_FAILURE;
            goto end;
        }
    }

end:
    return program_options;

arg_error:
    fprintf(stderr, "Invalid argument to option -%c: %s\n", the_option, optarg);
    program_options.action = command_action::HELP;
    program_options.exit_code = EXIT_FAILURE;
    goto end;
}

#endif // RTDAG_COMMAND_H
