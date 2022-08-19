/**
 * @author Alexandre Amory, ReTiS Lab, Scuola Sant'Anna, Pisa, Italy.
 * @brief POSIX Shared memmory IPC with blocking push once the queue size limit is reached
 * @version 0.1
 * @date 2022-07-06
 * 
 * @copyright Copyright (c) 2022
 * 
 * It uses shared memory among the processes and semaphores to sync that access and full/empty signals.
 * Overview of POSIX shared memory: https://www.man7.org/linux/man-pages/man7/shm_overview.7.html 
 * Overview of POSIX named semaphores: https://man7.org/linux/man-pages/man7/sem_overview.7.html
 * 
 *   "On Linux, named semaphores are created in a virtual filesystem, normally mounted under /dev/shm, 
 *      with names of the form sem.somename."
 * 
 * TODO:
 *   - use timed sema: sem_timedwait
 * 
 * based on these examples:
 *  - shared mem c fork and semaphores
 *    - https://github.com/LiuRuichen/Operating-System/blob/main/%E7%BB%BC%E5%90%88%E5%AE%9E%E9%AA%8C%E4%B8%80/%E4%BB%A3%E7%A0%81%E6%96%87%E4%BB%B6/%E7%94%9F%E4%BA%A7%E8%80%85%E6%B6%88%E8%B4%B9%E8%80%85%E9%97%AE%E9%A2%98/Customer.c
 *    - https://github.com/LiuRuichen/Operating-System/blob/main/%E7%BB%BC%E5%90%88%E5%AE%9E%E9%AA%8C%E4%B8%80/%E4%BB%A3%E7%A0%81%E6%96%87%E4%BB%B6/%E7%94%9F%E4%BA%A7%E8%80%85%E6%B6%88%E8%B4%B9%E8%80%85%E9%97%AE%E9%A2%98/Producer.c
 *    - https://github.com/LiuRuichen/Operating-System/blob/e001cbf4f2303774e585ae13abc440bed8c1f4c4/%E7%BB%BC%E5%90%88%E5%AE%9E%E9%AA%8C%E4%B8%80/%E4%BB%A3%E7%A0%81%E6%96%87%E4%BB%B6/%E7%94%9F%E4%BA%A7%E8%80%85%E6%B6%88%E8%B4%B9%E8%80%85%E9%97%AE%E9%A2%98/shm_com_sem.h
 *  - similar one
 *    - https://github.com/tibra27/OS-LAB/blob/master/Lab7(7-3-19)/PRODUCER_CONSUMER/pcsm4.c
 *  - https://gist.githubusercontent.com/PolarNick239/f727c0cd923398dc397a05f515452123/raw/b8f172e98d1b2c803716ecfc097c2039dbcd0cd0/blocking_queue.h
 *  - https://wang-yimu.com/a-tutorial-on-shared-memory-inter-process-communication/
 *    - https://github.com/yimuw/yimu-blog/tree/master/comms/shared_mem* 
 */
#ifndef CIRCULAR_SHM_H_
#define CIRCULAR_SHM_H_

#include <array>
#include <cassert>
#include <cstring>
#include <unistd.h>  // ftruncate
// share mem
#include <sys/mman.h>
#include <sys/shm.h>
// sema
#include <fcntl.h>           /* For O_* constants */
#include <semaphore.h>

#include "circular_comm.h"

template<class T,size_t TElemCount>
class circular_shm: public circular_comm<T,TElemCount>{
private:
    mutable std::array<T*, TElemCount> buf;
    // in this case, with one-line buffer, it's overkill to have two semaphores
    sem_t *                   sem_full;
    sem_t *                   sem_empty;
    // interprocess mutex to access the data
    sem_t *                   sem_mutex;

    // this->head and this->tail are ignored because they need to be shared among the processes
    typedef struct{
        unsigned head,tail;
    } head_tail_type;
    head_tail_type            *ht;

public:
    circular_shm(const std::string& name_): circular_comm<T,TElemCount>(name_) {
        // allocate TElemCount shared mem of sizeof(T) and save the pointers in the array
        char shm_name[32];
        assert(this->name.size() > 0);
        for(unsigned i =0; i < TElemCount;++i){
            snprintf(shm_name,32,"/%s_%u",this->name.c_str(),i);
            assert(strlen(shm_name) < 31);
            // save the pointer to the shared memory
            this->buf[i] = (T*)allocate_shm(shm_name,sizeof(T));
        }

        snprintf(shm_name,32,"%s_ht",this->name.c_str());
        ht = (head_tail_type*)allocate_shm(shm_name,sizeof(head_tail_type));
        
        // create the semas
        std::string sema_name = this->name + "_full";
        // it says the buffer starts 'not full', with 0 positions occupied
        sem_full = sem_open(sema_name.c_str(), O_CREAT, 0644, 0);
        assert(sem_full != SEM_FAILED);
        sema_name = this->name + "_empty";
        // it says the buffer starts empty, with TElemCount positions occupied
        /* attention to this passage of 
            https://man7.org/linux/man-pages/man3/sem_open.3.html
            The value argument specifies the initial value for the new semaphore.  If
            O_CREAT is specified, and a semaphore with the given name already
            exists, then mode and value are ignored.
        */
        sem_empty = sem_open(sema_name.c_str(),O_CREAT, 0644, TElemCount);
        assert(sem_empty != SEM_FAILED);
        sema_name = this->name + "_mutex";
        sem_mutex = sem_open(sema_name.c_str(),O_CREAT, 0644, 1);
        assert(sem_mutex != SEM_FAILED);
        // debug("circular_shm");
    }

    ~circular_shm(){
        int ret;
        (void) ret; // unused in release mode
        // unmap the shared memories
        for(unsigned i =0; i < TElemCount;++i){
            ret = munmap((void*)this->buf[i], sizeof(T)*TElemCount);
            assert(ret==0);
        }
        ret = munmap((void*)this->ht, sizeof(head_tail_type));
        assert(ret==0);
        // unlink the semas
        std::string sema_name = this->name + "_full";
        // unlink will avoid other processes to get access this sema name 
        // but the sema still exists until:
        //  - the same counts to 0 or
        //  - all processes that had previouly opened it, 
        //  had closed this sema
        //
        // sem_unlink cannot be check sem_unlink because it would rise an error when
        // the 2nd process (i.e. the sender or the receiver) tries to unlink this
        sem_unlink(sema_name.c_str());
        sema_name = this->name + "_empty";
        sem_unlink(sema_name.c_str());
        sema_name = this->name + "_mutex";
        sem_unlink(sema_name.c_str());
        ret = sem_close(sem_full);
        assert(ret==0);
        ret = sem_close(sem_empty);
        assert(ret==0);
        ret = sem_close(sem_mutex);
        assert(ret==0);
    }    

    void push(const T& data) noexcept{
        int ret;
        (void) ret; // unused in release mode
        // debug("push init");
        ret = sem_wait(sem_empty);
        assert(ret==0);
        ret = sem_wait(sem_mutex);
        assert(ret==0);
        memcpy(this->buf[this->ht->head], &data, sizeof(T));
        this->ht->head = (this->ht->head + 1) % this->capacity();
        this->empty = false;
        this->full = this->ht->head == this->ht->tail;            
        ret = sem_post(sem_mutex);
        assert(ret==0);
        ret = sem_post(sem_full);
        assert(ret==0);
        // debug("push end");
    }

    void pop(T &data) noexcept{
        int ret;
        (void) ret; // unused in release mode
        // debug("pop init");
        ret = sem_wait(sem_full);
        assert(ret==0);
        ret = sem_wait(sem_mutex);
        assert(ret==0);
        memcpy(&data, this->buf[this->ht->tail], sizeof(T));
        this->ht->tail = (this->ht->tail + 1) % this->capacity();
        this->full = false;
        this->empty = this->ht->head == this->ht->tail;
        ret = sem_post(sem_mutex);
        assert(ret==0);
        ret = sem_post(sem_empty);
        assert(ret==0);
        // debug("pop end");
    }


    void reset() noexcept{
        // TODO: not implemented
        printf("ERROR: circular_shm::size is not implemented");
        assert(0);
        // TODO: it misses the initialization of the full/empty semaphores
        // how to do it ? i have to close and open the sema again ?
        // I was not able to find a 'sem_init' for named sema. Only for unamed sema
        sem_wait(sem_mutex);
        this->ht->head = this->ht->tail;
        this->full = false;
        this->empty = true;
        sem_post(sem_mutex);
    }


    bool is_empty() const noexcept{
        // Can have a race condition in a multi-threaded application
        sem_wait(sem_empty);
        // if head and tail are equal, we are empty
        bool res = (!this->full && (this->ht->head == this->ht->tail));
        sem_post(sem_empty);
        return res;
    }

    bool is_full() const noexcept{
        // If tail is ahead the head by 1, we are full
        return this->full;
    }

    size_t size() const noexcept{
        // TODO: not implemented
        printf("ERROR: circular_shm::size is not implemented");
        assert(0);
        return 0;
    }
private:

    void debug(const char * func_name) const{
        int value; 
        int ret;
        (void) ret; // unused in release mode
        ret = sem_getvalue(sem_empty, &value);
        assert(ret==0);
        printf("[%s] Empty semaphore is %d\n", func_name, value);
        ret = sem_getvalue(sem_full, &value);
        assert(ret==0);
        printf("[%s] Full semaphore is %d\n", func_name, value);
        ret = sem_getvalue(sem_mutex, &value);
        assert(ret==0);
        printf("[%s] Mutex semaphore is %d\n", func_name, value);      
        printf("[%s] H & T: %u, %u\n\n\n", func_name, this->ht->head, this->ht->tail);  
    }
    // create the shared memory in the constructor
    void* allocate_shm(const std::string& shmem_name, const unsigned size_bytes)
    {
        int fd = -1;
        fd = shm_open(shmem_name.c_str(), O_CREAT | O_RDWR, 0666);

        if (fd == -1) {
            perror("ERROR: shm_open");
            return nullptr;
        }
        if (ftruncate(fd, size_bytes)) {
            printf ("ERROR: ftruncate\n");
            close(fd);
            return nullptr;
        }

        void* ret = mmap(0, size_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);

        if (ret == MAP_FAILED) {
            printf ("ERROR: mmap\n");
            return nullptr;
        }
        return ret;
    }

};

#endif
