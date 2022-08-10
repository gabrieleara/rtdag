#ifndef INPUT_YAML_H_
#define INPUT_YAML_H_

/*
This is a wrapper classe that reads the dag configuration from a YAML file. 

despite input_header, this approach does not required to recompile the rt-dag for every new scenario.
When dealing with a more complex setup where it's required to run hundreds of scenarios,
this becomes cumbersome. It gets even more complex because it requires cross-compilation
to build all the required scenarios
*/

// TODO: implemente a proper base class 
//#include "input_wrapper.h"
// #include <memory>
#include <iostream>
#include <yaml-cpp/yaml.h>

using namespace std;

// class input_yaml: public input_wrapper{
class input_yaml{

public:
    YAML::Node inputs;

    // input_yaml(const char* fname_): input_wrapper(fname_) {}
    input_yaml(const char* fname_){ 

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
        cout << "Reading name: " << inputs["dag_name"] << endl;
        exit(EXIT_FAILURE);

    }

    const char *    get_dagset_name() const { return "";}
    unsigned  get_n_tasks() const { return 0;}
    unsigned  get_n_edges() const { return 0;}
    unsigned  get_max_out_edges() const { return 0;}
    unsigned  get_max_in_edges() const { return 0;}
    unsigned  get_msg_len() const { return 0;}
    unsigned  get_repetitions() const { return 0;}
    unsigned long get_period() const { return 0;}
    unsigned long get_deadline() const { return 0;}
    const char *    get_tasks_name(unsigned t) const { return "";}
    unsigned long  get_tasks_wcet(unsigned t) const { return 0;}
    unsigned long  get_tasks_rel_deadline(unsigned t) const{ return 0;}
    unsigned  get_tasks_affinity(unsigned t) const { return 0;}
    unsigned  get_adjacency_matrix(unsigned t1,unsigned t2) const { return 0;}

};

#endif // INPUT_YAML_H_
