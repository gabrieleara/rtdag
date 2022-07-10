#ifndef SHARED_MEM_TYPE_H_
#define SHARED_MEM_TYPE_H_

#define MAX_SHM_BUFFER_SIZE 256

/*
 This is the data type sent among the tasks.
 This data type could be changed as long as its size is defined in compile time
*/
class shared_mem_type{
private:
    char data[MAX_SHM_BUFFER_SIZE];
public:
    shared_mem_type() {data[0]=0;}

    // build a default message and adjust its size.
    // So, it can either trunk the default message if 'msg_size' is lower or
    // it can fill the message with spaces to complete the message size.
    // msg_size includes the string terminator.
    void set(const char * name, const unsigned msg_num, const unsigned msg_size){
        assert (msg_size <= MAX_SHM_BUFFER_SIZE);
        // build the default message
        sprintf(data,"Message from '%s', id: %u",name, msg_num);
        // make sure that MAX_SHM_BUFFER_SIZE was not set too low
        assert (this->size() <= MAX_SHM_BUFFER_SIZE);
        if (this->size() >= msg_size){
            // trunc the message to meet the required size
            data[msg_size-1]=0;
        }else{
            // extend the msg size filling the string with white spaces
            unsigned i;
            for (i=this->size()-1; i<msg_size-1; ++i){
                data[i] = ' ';
            }
            data[i] = 0;
        }
        // make sure the strlen is of the required size
        assert(this->size() == msg_size);
    }

    char * get(){ return data;}

    unsigned size() const { return strlen(data)+1;}

    // TODO: improve this additional methods
    // operator '='         ==> replace memcpy by simple assignement operator
    // operator '<<'        ==> easy debugging

};

#endif
