#include <cstdlib>

#include "rtdag_run.h"
#include "rtdag_calib.h"
#include "rtdag_command.h"

#include "rtgauss.h"

// FIXME: tunable matrix size also for CPU calibration!

//TODO:
extern int calibrate(unsigned int t);
extern int test_calibration(unsigned int t);
extern int run_dag(const std::string);

float expected_wcet_ratio_override = 0.0;

int main(int argc, char *argv[]) {
    auto program_options = parse_args(argc, argv);
    expected_wcet_ratio_override = program_options.expected_wcet_ratio;

    switch (program_options.action) {
    case command_action::HELP:
        usage(argv[0]);
        return program_options.exit_code;

    case command_action::CALIBRATE:
        rtgauss_init(4, RTGAUSS_CPU, 0);
        return calibrate(program_options.duration_us);

    case command_action::TEST:
        rtgauss_init(4, RTGAUSS_CPU, 0);
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
