#ifndef RTDAG_TASK_H
#define RTDAG_TASK_H

#include <barrier>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "newstuff/mqueue.h"
#include "newstuff/schedutils.h"
#include "periodic_task.h"
#include "rtdag_calib.h"
#include "rtgauss.h"
#include "time_aux.h"

class Dag {
public:
    const std::string name;
    const microseconds period;
    const microseconds e2e_deadline;
    const s64 num_activations;

    // For some reason I need to specify <>
    std::barrier<> barrier;

    // One per task. The originator technically does not have any, but we
    // will use it to exchange the start time of the DAG with the sink
    // task.
    //
    // Using unique_ptr because MultiQueue is not movable (due to
    // std::mutex and other attributes), which is not possible as a vector
    // element.
    std::vector<std::unique_ptr<MultiQueue>> in_queues;

    // The edge connections between tasks (reference the in_queues above)
    std::vector<Edge> edges;

    // Used to store the start time of the dag.
    //
    // NOTICE: this variable is NOT lock protected because ONLY THE DAG
    // ORIGINATOR TASK WILL BE ABLE TO EXECUTE WHEN ACCESSED ON WRITE. All
    // other tasks will be blocked waiting for the dag originator to wakeup
    // and THE SINK TASK WILL WAKE UP THE ORIGINATOR AFTER FINISHING, so NO
    // OVERLAP BETWEEN THE ORIGINATOR AND THE SINK IS POSSIBLE, hence the
    // variable can be accessed freely without any (additional) lock.
    struct timespec start_time;

    // Used to release a new instance of the dag
    MultiQueue *start_dag;

    // All the response times
    std::vector<microseconds> response_times;

    Dag(const std::string &name, microseconds period, microseconds e2e_deadline,
        s64 num_activations, s32 ntasks) :
        name(name),
        period(period),
        e2e_deadline(e2e_deadline),
        num_activations(num_activations),
        barrier(ntasks),
        response_times(num_activations) {}
};

class Task {
public:
    Dag &dag;

    const std::string name;
    const std::string type;
    const sched_info scheduling;
    const int cpu;

    std::vector<Edge *> in_buffers;
    std::vector<Edge *> out_buffers;

    period_info pinfo;

#if RTDAG_MEM_ACCESS == ON
    // This volatile variable is used to avoid optimizing away all the
    // memory operations.
    volatile char checksum = 0;
#endif

private:
    void task_body(unsigned seed);
    void common_init();
    void loop_body_before(int iter);
    void loop_body_after(int iter, const struct timespec &duration);
    void common_exit();

protected:
    virtual void do_init() = 0;
    virtual void do_loop_work(int iter) = 0;
    virtual void do_exit() = 0;

public:
    Task(Dag &dag, const std::string &name, const std::string &type,
         const sched_info &scheduling, int cpu,
         const std::vector<Edge *> &in_edges, std::vector<Edge *> out_edges) :
        dag(dag),
        name(name),
        type(type),
        scheduling(scheduling),
        cpu(cpu),
        in_buffers(in_edges),
        out_buffers(out_edges) {}

    virtual ~Task() = default;

    // TODO: implement correctly the full process launcher
    std::thread start(int seed) {
        return std::thread(&Task::task_body, this, seed);
    }

    inline bool is_originator() const {
        return in_buffers.size() == 0;
    }

    inline bool is_sink() const {
        return out_buffers.size() == 0;
    }

    void print(std::ostream &os);
};

class GaussTask : public Task {
    // TODO: review all the types
    const microseconds wcet;
    const float ticks_per_us;

    const s32 matrix_size;
    const s32 omp_target;

public:
    GaussTask(Dag &dag, const std::string &name, const std::string &type,
              const sched_info &scheduling, int cpu,
              const std::vector<Edge *> &in_edges,
              std::vector<Edge *> out_edges, microseconds wcet,
              u64 expected_wcet_ratio, float ticks_per_us, s32 matrix_size,
              s32 omp_target) :
        Task(dag, name, type, scheduling, cpu, in_edges, out_edges),
        wcet(wcet.count() * expected_wcet_ratio),
        ticks_per_us(ticks_per_us),
        matrix_size(matrix_size),
        omp_target(omp_target) {}

    virtual rtgauss_type get_rtgauss_type() const = 0;

    void do_init() override {
        rtgauss_init(matrix_size, get_rtgauss_type(), omp_target);

        // Pre-load code on the CPU/GPU/... for fast execution later on!
        int retv = waste_calibrate(); // FIXME: implement it differently!!
        (void)retv;
        LOG(DEBUG, "Waste calibrate value %d\n", retv);
    }

    void do_loop_work(int iter) override {
        LOG(INFO, "task %s (%u): running the processing step for %lu * %f\n",
            name.c_str(), iter, wcet.count(), ticks_per_us);
        Count_Time_Ticks(wcet, ticks_per_us);
    }

    void do_exit() override {
        // Do nothing is fine
    }
};

class CPUTask : public GaussTask {
public:
    using GaussTask::GaussTask;

    rtgauss_type get_rtgauss_type() const override {
        return RTGAUSS_CPU;
    }
};

#if RTDAG_OMP_SUPPORT == ON
class OMPTask : public GaussTask {
public:
    using GaussTask::GaussTask;

    rtgauss_type get_rtgauss_type() const override {
        return RTGAUSS_OMP;
    }
};
#endif

#endif // RTDAG_TASK_H
