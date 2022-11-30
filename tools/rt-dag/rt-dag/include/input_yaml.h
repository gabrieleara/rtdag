#ifndef INPUT_YAML_H_
#define INPUT_YAML_H_

/*
This is a implementation of the input_wrapper class that reads the dag configuration from a YAML file.

despite input_header, this approach does not required to recompile the rt-dag for every new scenario.
When dealing with a more complex setup where it's required to run hundreds of scenarios,
this becomes cumbersome. It gets even more complex because it requires cross-compilation
to build all the required scenarios
*/

#include "input_wrapper.h"
#include <string>
#include <yaml-cpp/yaml.h>

using namespace std;

class input_yaml: public input_wrapper{
private:
    string dag_name;
    string tasks_name[32];
    string tasks_type[32];
    YAML::Node inputs;
public:
    input_yaml(const char* fname_): input_wrapper(fname_) {

        try{
            inputs = YAML::LoadFile(fname_);
        }
        catch (YAML::ParserException & e)
        {
            cerr << "Exception :: " << e.what() << endl;
            cerr << "ERROR: Syntax error in the SW YAML " << fname_ << endl;
            exit(EXIT_FAILURE);
        }

        if (!inputs["dag_name"]){
            cerr << "ERROR: dag_name attribute is missing. Is this the correct rt-dag YAML file ?!?!\n\n";
            exit(EXIT_FAILURE);
        }
        //cout << "Reading name: " << inputs["dag_name"] << endl;
        dag_name = inputs["dag_name"].as<string>();
        const int ntasks = get_n_tasks();
        if (ntasks >= (int) (sizeof(tasks_name)/sizeof(tasks_name[0]))) {
            cerr << "Max # of tasks exceeded in input YAML" << endl;
            exit(EXIT_FAILURE);
        }
        for (int i=0;i<ntasks;i++){
            tasks_name[i] = inputs["tasks_name"][i].as<string>();
            // by default, all tasks as 'cpu' type, to keep compatibility w older rt-dag input files
            tasks_type[i] = "cpu";
            if (inputs["tasks_type"])
                tasks_type[i] = inputs["tasks_type"][i].as<string>();
        }
    }

    const char *  get_dagset_name() const override { return dag_name.c_str();}
    unsigned  get_n_tasks() const override { return inputs["n_tasks"].as<unsigned>();}
    unsigned  get_n_edges() const override { return inputs["n_edges"].as<unsigned>();}
    unsigned  get_n_cpus() const override { return inputs["n_cpus"].as<unsigned>();}
    unsigned  get_max_out_edges() const override { return inputs["max_out_edges"].as<unsigned>();}
    unsigned  get_max_in_edges() const override { return inputs["max_in_edges"].as<unsigned>();}
    unsigned  get_msg_len() const override { return inputs["max_msg_len"].as<unsigned>();}
    unsigned  get_repetitions() const override { return inputs["repetitions"].as<unsigned>();}
    unsigned long get_period() const override { return inputs["dag_period"].as<unsigned long>();}
    unsigned long get_deadline() const override { return inputs["dag_deadline"].as<unsigned long>();}
    unsigned long get_hyperperiod() const override { return inputs["hyperperiod"].as<unsigned long>();}
    const char *  get_tasks_name(unsigned t) const override { return tasks_name[t].c_str();}
    const char *  get_tasks_type(unsigned t) const override { return tasks_type[t].c_str();}
    int get_fred_id(unsigned t) const override { return inputs["fred_id"] ? inputs["fred_id"][t].as<int>() : -1;}
    unsigned long get_tasks_runtime(unsigned t) const override { return inputs["tasks_runtime"][t].as<unsigned long>();}
    unsigned long get_tasks_wcet(unsigned t) const override { return inputs["tasks_wcet"][t].as<unsigned long>();}
    unsigned long get_tasks_rel_deadline(unsigned t) const override { return inputs["tasks_rel_deadline"][t].as<unsigned long>();}
    unsigned  get_tasks_affinity(unsigned t) const override { return inputs["tasks_affinity"][t].as<int>();}
    unsigned  get_adjacency_matrix(unsigned t1,unsigned t2) const override { return inputs["adjacency_matrix"][t1][t2].as<unsigned>();}
    float get_expected_wcet_ratio() const override { return inputs["expected_wcet_ratio"].as<float>(1.0f);}
};

#endif // INPUT_YAML_H_
