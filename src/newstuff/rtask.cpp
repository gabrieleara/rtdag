#include "newstuff/rtask.h"
#include "logging.h"
#include "periodic_task.h"
#include <string_view>

#include <cassert>
#include <fstream>
#include <istream>
#include <ostream>
#include <span>

// ------------------------- HELPER FUNCTIONS -------------------------- //

static inline void task_set_name(const std::string_view &sname) {
    // According to pthread_setname_np(3), the string must be limited to 16
    // characters, including the NULL-termination!
    char name[16];
    int ncopied = sname.copy(name, 15);
    name[ncopied] = '\0';

    if (int res = pthread_setname_np(pthread_self(), name)) {
        (void)res;
        LOG(ERROR, "Could not set thread name!\n");
        exit(EXIT_FAILURE);
    };
}

static inline int task_pin_thread(const cpu_set_t &cpuset) {
    return pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
}

static inline int task_pin_process(const cpu_set_t &cpuset) {
    return sched_setaffinity(getpid(), sizeof(cpuset), &cpuset);
}

static inline void task_pin(int cpu) {
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);

    if (int res =
#if RTDAG_TASK_IMPL == TASK_IMPL_THREAD
            task_pin_thread(cpuset)
#else
            task_pin_process(cpuset)
#endif
    ) {
        (void)res;
        LOG(ERROR, "Could not pin to core %d!\n", cpu);
        exit(EXIT_FAILURE);
    }
}

// static inline void task_open_exec_time_file(task_data &task,
//                                             ofstream &exec_time_f) {
//     std::stringstream ss;
//     // TODO: this is not the correct way in C++ to build a valid path!
//     ss << task.dag.name << "/";
//     ss << task.name << ".log";
//     std::string fname = ss.str();
//     // FIXME: Why oh why is it every file open in append in this
//     // application?
//     exec_time_f.open(fname, std::ios_base::app);
//     if (!exec_time_f) {
//         LOG(ERROR, "execution time file '%s' not created\n", fname.c_str());
//         exit(EXIT_FAILURE);
//     }
//     // the 1st line is the task relative deadline. all the following lines
//     // are actual execution times
//     exec_time_f << task.sched.deadline << '\n';
// }

// ------------------------- MEMBER FUNCTIONS -------------------------- //

void Task::task_body() {
    do_init();
    common_init();

    for (int i = 0; i < dag.num_activations; ++i) {
        std::chrono::microseconds before, after, duration;

        loop_body_before(i);
        before = std::chrono::microseconds(micros());

        do_loop_work(i);
        after = std::chrono::microseconds(micros());
        duration = after - before;

        loop_body_after(i, duration);
    }

    common_exit();
    do_exit();
}

void wait_on_barrier(std::barrier<> &barrier, const std::string &who) {
#if TASK_IMPL == TASK_IMPL_THREAD
    // wait for all threads in the DAG to have been started up to this point
    LOG(DEBUG, "barrier_wait()ing on: %p for task %s\n", (void *)&barrier,
        who.c_str());

    barrier.arrive_and_wait();

    LOG(DEBUG, "barrier_wait() returned:\n");
#endif
}

#define TIMESPEC_FORMAT "%ld.%.9ld"

void period_init(period_info &pinfo, std::chrono::microseconds period) {
    pinfo_init(&pinfo, std::chrono::nanoseconds(period).count());
}

void align_deadlines(period_info &pinfo) {
    using namespace std::chrono_literals;

    // Wait for 100ms to make sure that in-kernel CBS deadlines are
    // aligned with the absolute deadlines in pinfo.
    std::chrono::milliseconds waitfor = 100ms;

    LOG(DEBUG, "waiting for %ld ms...\n", waitfor.count());
    pinfo_sum_and_wait(&pinfo, std::chrono::nanoseconds(waitfor).count());
    LOG(DEBUG, "woken up: pinfo.next_period: " TIMESPEC_FORMAT " s\n",
        pinfo.next_period.tv_sec, pinfo.next_period.tv_nsec);
}

void Task::common_init() {
    task_set_name(name);

    if (cpu >= 0) {
        task_pin(cpu);
    }

    // task_clean_buffers(data);

    scheduling.set();

    wait_on_barrier(dag.barrier, name);

    if (is_originator()) {
        period_init(pinfo, dag.period);
    }

    wait_on_barrier(dag.barrier, name);

    if (is_originator()) {
        align_deadlines(pinfo);
    }
}

std::chrono::microseconds get_next_period(struct period_info *pinfo) {
    using ms = std::chrono::microseconds;

    struct timespec &period = pinfo->next_period;
    return ms(std::chrono::seconds(period.tv_sec)) +
           std::chrono::duration_cast<ms>(
               std::chrono::nanoseconds(period.tv_nsec));
}

#if RTDAG_MEM_ACCESS == ON
// used to mimic the buffer memory reads
static char read_input_buffer(std::span<char> buffer) {
    char checksum = 0;
    for (unsigned i = 0; i < buffer.size(); ++i) {
        checksum ^= buffer[i];
    }
    return checksum;
}
#endif

void wait_incoming_messages(Task &task, int iter) {
    if (task.in_buffers.size()) {
        // All the input buffers share the same MultiQueue reference, so we
        // can just wait on the first one
        //
        // FIXME: remove the size argument from the pop
        task.in_buffers[0]->mq.pop(NULL, task.in_buffers.size());

        // Check that all the buffers have sent the right amount of data
        for (size_t i = 0; i < task.in_buffers.size(); ++i) {
            // NOTE: CHECKED ONLY IN DEBUG MODE
            assert(strlen(task.in_buffers[i]->msg.data()) ==
                   task.in_buffers[i]->msg.size() - 1);

#if RTDAG_MEM_ACCESS == ON
            // This is a dummy code (a checksum calculation w xor) to
            // mimic the memory reads required by the task model
            task.checksum ^= read_input_buffer(task.in_buffers[i]->msg);
#endif

            // To avoid printing too many characters if the buffer is very
            // long, we limit to the first 50 characters.
            LOG(DEBUG,
                "task %s (%u), buffer n%d_n%d(%lu): got message: '%.50s'\n",
                task.name.c_str(), iter, task.in_buffers[i]->from,
                task.in_buffers[i]->to, strlen(task.in_buffers[i]->msg.data()),
                task.in_buffers[i]->msg.data());
        }
    }
}

void Task::loop_body_before(int iter) {
    // A variable is set at the beginning of each period by the originator
    // and read at the end of each period by the sink, to calculate overall
    // response time.

    if (is_originator()) {
        std::chrono::microseconds now = get_next_period(&pinfo);

        // HACK: this is a terrible idea and it should be fixed!!
        dag.start_time.push(0, (void *)now.count());

        LOG(DEBUG, "task %s (%u): dag start time %lu\n", name.c_str(), iter,
            now.count());
    }

    wait_incoming_messages(*this, iter);
}

void write_to_queue(const char *from, int iter, char *buffer, int size) {
#if RTDAG_MEM_ACCESS != ON
    (void)from;
    (void)iter;
    (void)buffer;
    (void)size;
#else
    // The printf helps debug from which task writes where, but it is a
    // performance penalty compared to simply using memcpy or memset.
    // Consider replacing with those if you experience performance issues
    // in this step.

    int len =
        (int)snprintf(buffer, size, "Message from %s, iter: %d", from, iter);
    if (len < size) {
        memset(buffer + len, '.', size - len);
        buffer[size - 1] = 0;
    }
#endif
}

void Task::loop_body_after(int iter, std::chrono::microseconds duration) {
    // Push the values into each queue
    for (size_t i = 0; i < out_buffers.size(); ++i) {
        write_to_queue(name.c_str(), iter, out_buffers[i]->msg.data(),
                       out_buffers[i]->msg.size());

        // The values pushed in the multi-queue are meaningless, on the
        // read side we always go check the ->msg content anyway...
        out_buffers[i]->mq.push(out_buffers[i]->push_idx,
                                out_buffers[i]->msg.data());

        // To avoid printing too many characters if the buffer is very
        // long, we limit to the first 50 characters.
        LOG(DEBUG,
            "task %s (%u): buffer n%d_n%d, size %lu, sent message: '%.50s'\n",
            name.c_str(), iter, out_buffers[i]->from, out_buffers[i]->to,
            strlen(out_buffers[i]->msg.data()), out_buffers[i]->msg.data());
    }

#ifndef NDEBUG
    // FIXME: implement this stuff as well

    // write the task execution time into its log file
    exec_time_f << duration << endl;
    if (duration > task.deadline) {
        printf("ERROR: task %s (%u): task duration %lu > deadline %lu!\n",
               task_name, iter, duration, task.deadline);
        // TODO: stop or continue ?
    }
#endif // NDEBUG

    if (is_sink()) {
        s64 last_dag_start;

        // FIXME: there's something seriously wrong here
        dag.start_time.pop((void **)&last_dag_start, 1);

        duration = std::chrono::microseconds(micros()) -
                   std::chrono::microseconds(last_dag_start);

        LOG(INFO, "task %s (%u): dag duration %lu us = %lu ms = %lu s\n\n",
            name.c_str(), iter, duration.count(),
            std::chrono::duration_cast<std::chrono::milliseconds>(duration)
                .count(),
            std::chrono::duration_cast<std::chrono::seconds>(duration).count());

        dag.response_times[iter] = duration;

        if (duration > dag.e2e_deadline) {
            // we do expect a few deadline misses, despite all
            // precautions, we'll find them in the output file
            LOG(ERROR,
                "ERROR: dag deadline violation detected in iteration "
                "%u. duration %ld us\n",
                iter, duration.count());
        }
    }

    if (is_originator()) {
        // Wait for the next period activation
        pinfo_sum_period_and_wait(&pinfo);
    }
}

std::fstream open_append(const std::string &fname, bool &existed) {
    std::fstream os;
    existed = false;

    // First open will NOT create the file because ios_base::in is in the
    // flags
    os.open(fname, ios_base::out | ios_base::in);
    if (os.is_open()) {
        existed = true;
    }

    // We have to close and re-open in append this time
    os.clear();
    os.open(fname, ios_base::out | ios_base::app);

    return os;
}

void Task::common_exit() {
#ifndef NDEBUG
    exec_time_f.close();
#endif // NDEBUG

    if (is_sink()) {
        // FIXME: change this to avoid creating the output directory
        std::stringstream ss;
        ss << dag.name << "/" << dag.name << ".log";

        bool existed;
        std::fstream os = open_append(ss.str(), existed);

        if (existed) {
            // We will write on the first line the e2e deadline
            os << dag.e2e_deadline << '\n';
        }

        for (const auto &rt : dag.response_times) {
            os << rt.count() << "\n";
        }
    }

#if RTDAG_MEM_ACCESS == ON
    // Don't remove this print. otherwise the logic to read the memory will
    // be optimized in Release mode.
    //
    // GA: I actually think we can remove this print, since the checksum
    // variable is volatile.
    //
    printf("%c\n", checksum);
#endif
}

void Task::print(std::ostream &os) {
    os << name << ", ";
    os << "type: " << type << ", ";
    os << "runtime: " << scheduling.runtime().count() << "ns, ";
    os << "deadline: " << scheduling.deadline().count() << "ns, ";
    os << "affinity: " << cpu << '\n';

    os << " ins: ";
    for (const auto &edge_ptr : in_buffers) {
        os << "n" << edge_ptr->from << "_n" << edge_ptr->to << ", ";
    }
    os << '\n';

    os << " outs: ";
    for (const auto &edge_ptr : out_buffers) {
        os << "n" << edge_ptr->from << "_n" << edge_ptr->to << ", ";
    }
    os << '\n';
}
