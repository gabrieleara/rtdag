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
        for (int i=0;i<ntasks;i++){
            tasks_name[i] = inputs["tasks_name"][i].as<string>();
        }
    }

    const char *  get_dagset_name() const { return dag_name.c_str();}
    unsigned  get_n_tasks() const { return inputs["n_tasks"].as<unsigned>();}
    unsigned  get_n_edges() const { return inputs["n_edges"].as<unsigned>();}
    unsigned  get_max_out_edges() const { return inputs["max_out_edges"].as<unsigned>();}
    unsigned  get_max_in_edges() const { return inputs["max_in_edges"].as<unsigned>();}
    unsigned  get_msg_len() const { return inputs["max_msg_len"].as<unsigned>();}
    unsigned  get_repetitions() const { return inputs["repetitions"].as<unsigned>();}
    unsigned long get_period() const { return inputs["dag_period"].as<unsigned long>();}
    unsigned long get_deadline() const { return inputs["dag_deadline"].as<unsigned long>();}
    const char *  get_tasks_name(unsigned t) const { return tasks_name[t].c_str();}        
    unsigned long get_tasks_wcet(unsigned t) const { return inputs["tasks_wcet"][t].as<unsigned long>();}
    unsigned long get_tasks_rel_deadline(unsigned t) const{ return inputs["tasks_rel_deadline"][t].as<unsigned long>();}
    unsigned  get_tasks_affinity(unsigned t) const { return inputs["tasks_affinity"][t].as<unsigned>();}
    unsigned  get_adjacency_matrix(unsigned t1,unsigned t2) const { return inputs["adjacency_matrix"][t1][t2].as<unsigned>();}
};

#endif // INPUT_YAML_H_
