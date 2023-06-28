#!/usr/bin/env python3

""" yaml2yaml converter

Converts the YAML files (both input and output YAML files)from the power optimization tools into a
YAML file for *rt-dag*, representing the DAG application.

From the input YAML it loads the DAG model. From the output YAML file it loads the task placement and cpu freq.

If the input YAML consists of multiple independent DAGs, then yaml2yaml will generate a rt-dag YAML file for each
DAG since rt-dag launches a single DAG. When deadling multiple DAGs, the solution is to run rt-dag multiple times
with different input configuration files.

Authors:
    * Gabriele Ara

"""
import os
import sys
import argparse
from ruamel.yaml import YAML
import math
import numpy

SUPPORTED_TASK_TYPES = [
    'cpu',
    'fred',
    'omp',
    # opencl,
]

# FIXME: for now it is based on the advertised (scaled) wcet in ns
FRED_IDS = {
     897615: 100, # sum_vec
     899473: 101, # xor_vec
    4381910: 102, # mul128
    1276500: 103, # mul64
}

yaml = YAML()
yaml.default_flow_style = None
yaml.sequence_dash_offset = 2


def write_yaml(fname, data):
    with open(fname, 'w') as stream:
        yaml.dump(data, stream)
# --


def read_yaml(fname):
    try:
        with open(fname, 'r') as stream:
            return yaml.load(stream)
    except yaml.YAMLError as e:
        print("ERROR: Parsing YAML file:", fname)
        print(e)
        sys.exit(1)
    except:
        print("ERROR: Loading YAML file:", fname)
        sys.exit(1)
# --

def lcm(*integers):
    a = integers[0]
    for b in integers[1:]:
        a = (a * b) // math.gcd(a,b)
    return a

def dag_hyperperiod(periods_list):
    return lcm(*periods_list)
# --


def to_us_safe(n_ns, ceil=False):
    if ceil:
        return math.ceil(int(n_ns / 1000))
    else:
        return math.floor(int(n_ns / 1000))
# --


def find_mapping(islands, dag_id, task_id):
    # Return (island_id, cpu_id)
    for island_id, island in enumerate(islands):
        for pu_id, tasks_list in enumerate(island['pus']):
            for t in tasks_list:
                if t[1] == dag_id and t[2] == task_id:
                    return island_id, pu_id
    assert False, "No mapping found!!"
# --


def get_cpu(platform_name, island, pu_id):
    if is_accelerator(platform_name, island):
        return 0 + pu_id

    if 'odroid' in platform_name:
        # Use capacity to determine the island
        capacity = island['capacity']

        if capacity < 1:
            # LITTLE
            return 0 + pu_id
        else:
            # BIG
            return 4 + pu_id

    elif 'zcu102' in platform_name:
        # Has only one island
        return 0 + pu_id
    elif 'xavier-agx' in platform_name:
        return 0 + pu_id

    assert False, "Unknown platform name"
# --


def is_accelerator(platform_name, island):
    if 'odroid' in platform_name:
        if island['capacity'] > 1:
            return True
    elif 'zcu102' in platform_name:
        if island['capacity'] > 1:
            return True
    elif 'xavier-agx' in platform_name:
        if len(island['pus']) == 1:
            return True
    else:
        assert False, "Unknown platform name"
    return False
# --


def generate_rtdag(dag, dag_id, dag_name, sol_data, iterations, hyperperiod):
    max_inputs = 0
    max_outputs = 0
    max_label_size = 0
    for t in range(len(dag['tasks'])):
        n_ins = len([e for e in dag['edge_list'] if e[0] == t])
        n_outs = len([e for e in dag['edge_list'] if e[1] == t])
        max_inputs = max(n_ins, max_inputs)
        max_outputs = max(n_outs, max_outputs)
        # TODO not yet implemented
        # max_label_size = e[2]

    # FIXME: truncating deadlines!!!

    n_tasks = len(dag['tasks'])
    # Each task is named taskXXX
    tasks_name = ["n{:03d}".format(t) for t in range(n_tasks)]
    tasks_wcet = [to_us_safe(task['wcet_ref'], False) for task in dag['tasks']]

    # TODO sol_data does not include dummy tasks

    tasks_runtime = []
    tasks_deadline = []
    tasks_type = []
    tasks_cpu = []
    fred_ids = []

    for task in sol_data['tasks']:
        # the id matches in a tuple (dag_id, task_id)
        if int(task['id'][0]) == dag_id:
            # found a valid task, assuming the tasks are in order this is
            # the n-th task of the current dag
            task_id = task['id'][1]
            assert task_id == len(tasks_runtime)

            t_runtime = to_us_safe(task['wcet'], False)
            t_deadline = to_us_safe(task['deadline'], False)

            # HACK: this is actually wrong, but for some files that
            # Francesco is sending me it must be done...
            if (tasks_wcet[task_id] < 1):
                tasks_wcet[task_id] = t_runtime

            island_id, pu_id = find_mapping(sol_data['islands'], dag_id, task_id)
            t_cpu = get_cpu(sol_data['platform_name'], sol_data['islands'][island_id], pu_id)

            # Task may be accelerated, check whether it is accelerated
            # or not

            # TODO extend this to work w other types of accelerators,
            # like OpenCL-based ones. there are cases where a hw task
            # is not selected. Thus, it needs to check whether it was
            # selected as hw or as cpu task
            if is_accelerator(sol_data['platform_name'], sol_data['islands'][island_id]):
                t_type = 'fred'
            else:
                t_type = 'cpu'

            if t_type == 'fred':
                try:
                    f_id = FRED_IDS[int(task['wcet'])] # in ns
                except:
                    # Key not found, assign id 102, which is the worst one
                    f_id = 102
            else:
                f_id = -1

            # t_runtime = max(t_runtime, 1024)
            # assert t_runtime >= 1024, "SCHED_DEADLINE does not support runtime < 1024!"

            tasks_type += [t_type]
            tasks_runtime += [t_runtime]
            tasks_deadline += [t_deadline]
            tasks_cpu += [t_cpu]
            fred_ids += [f_id]

    # --

    assert len(tasks_runtime) == n_tasks

    cpus_freq = []
    for i in sol_data['islands']:
        freq = i['frequency']  # in MHz
        if is_accelerator(sol_data['platform_name'], i):
            continue
        for pu_id, pu in enumerate(i['pus']):
            cpu = get_cpu(sol_data['platform_name'], i, pu_id)
            cpus_freq += [(cpu, freq)]

    # Use CPU id to sort and then select second element
    cpus_freq = [x[1] for x in sorted(cpus_freq, key=lambda x: x[0])]

    # FIXME: does not support message size!!!
    edge_matrix = numpy.zeros((n_tasks, n_tasks), dtype=int)
    for t1 in range(n_tasks):
        for t2 in range(n_tasks):
            edge_matrix[t1, t2] = [t1, t2] in dag['edge_list']
    edge_matrix = edge_matrix.tolist()

    # TODO: add comments, I know I can do it

    return {
        "dag_name": dag_name,
        "n_tasks": n_tasks,
        "n_edges": len(dag['edge_list']),
        # TODO: NOT CORRECT
        "n_cpus": len(cpus_freq),
        "max_out_edges": max_outputs,
        "max_in_edges": max_inputs,
        "max_msg_len": max_label_size + 1,
        "repetitions": iterations,
        "hyperperiod": to_us_safe(hyperperiod, False),
        "dag_period": to_us_safe(dag['activation_period'], False),
        "dag_deadline": to_us_safe(dag['deadline'], False),
        "tasks_name": tasks_name,
        "tasks_type": tasks_type,
        "tasks_wcet": tasks_wcet,
        "tasks_runtime": tasks_runtime,
        "tasks_rel_deadline": tasks_deadline,
        "tasks_affinity": tasks_cpu,
        "fred_id": fred_ids,
        # in MHz (can be ignored)
        "cpus_freq": cpus_freq,
        "expected_wcet_ratio": .95,
        # array of arrays (square matrix)
        "adjacency_matrix": edge_matrix,
    }


def main():
    # parsing arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('in_yaml', type=str,
                        help='Input YAML file with the DAG model.'
                        )
    parser.add_argument('sol_yaml', type=str,
                        help='Output YAML file with the task placement.'
                        )
    parser.add_argument('outdir', type=str,
                        help='Directory where to place the output files (one per dag).'
                        )
    parser.add_argument('-n', '--num-iters', type=int, default=10,
                        help='The number of times the DAG is executed. Default is 10.'
                        )
    parser.add_argument('-S', '--min-seconds', type=int, default=0,
                        help='The minimum time a DAG gets executed; overwrites --n if greater than 0 and --n is not sufficient to run for that amount of time. Default is 0.'
                        )
    args = parser.parse_args()

    # Read dag description and solution file
    dags = read_yaml(args.in_yaml)
    sol_data = read_yaml(args.sol_yaml)

    # FIXME: repetitions are the same for all the DAGs?

    # In ns
    hyperperiod = dag_hyperperiod(
        [int(d['activation_period']) for d in dags['dags']])
    repetitions = args.num_iters
    if args.min_seconds > 0:
        new_iter = int(math.ceil(args.min_seconds *
                       1_000_000_000) / hyperperiod)
        repetitions = new_iter if repetitions < new_iter else repetitions

    for dag_id, dag in enumerate(dags['dags']):
        dag_name = "dag{:d}".format(dag_id)
        rtdag = generate_rtdag(
            dag,
            dag_id,
            dag_name,
            sol_data,
            repetitions,
            hyperperiod)
        os.makedirs(args.outdir, exist_ok=True)
        write_yaml(args.outdir + "/" + dag_name + ".yaml", rtdag)


if __name__ == '__main__':
    sys.exit(main())
