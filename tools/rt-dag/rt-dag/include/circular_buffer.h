/**
 * @author Alexandre Amory, ReTiS Lab, Scuola Sant'Anna, Pisa, Italy.
 * @brief A thread-safe queue with blocking push once the queue size limit is reached.
 * @version 0.1
 * @date 2022-07-06
 * 
 * @copyright Copyright (c) 2022
 * 
 * based on : https://gist.githubusercontent.com/PolarNick239/f727c0cd923398dc397a05f515452123/raw/b8f172e98d1b2c803716ecfc097c2039dbcd0cd0/blocking_queue.h
 */
#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_

#include <mutex>
#include <condition_variable>

#include "circular_comm.h"

template<class T,size_t TElemCount>
class circular_buffer: public circular_comm<T,TElemCount>{
private:
    mutable std::array<T, TElemCount> buf;

    mutable std::mutex        m;
    // these are used to block both when reading (when empty) and writing (when full)
    std::condition_variable   item_added_event;
    std::condition_variable   item_removed_event;

public:
    // the only reason i need to pass empty name is to keep it compatible w circular_shm
    // even though this name is not user here
    circular_buffer(const std::string& name_=""): circular_comm<T,TElemCount>(name_) {}

    void push(const T& data) noexcept{
        {
            std::unique_lock<std::mutex> lock(m);
            while (this->full) {
                item_removed_event.wait(lock);
            }
            // TODO: T requires = operator
            this->buf[this->head] = data;
            this->head = (this->head + 1) % this->capacity();
            this->empty = false;
            this->full = this->head == this->tail;            
        }
        item_added_event.notify_one();
    }

    void pop(T &data) noexcept{
        {
            std::unique_lock<std::mutex> lock(m);
            while (this->empty) {
                item_added_event.wait(lock);
            }
            // TODO: T requires = operator
            data = this->buf[this->tail];
            this->tail = (this->tail + 1) % this->capacity();
            this->full = false;
            this->empty = this->head == this->tail;
        }
        item_removed_event.notify_one();
    }

    void reset() noexcept{
        std::unique_lock<std::mutex> lock(m);
        this->head = this->tail;
        this->full = false;
        this->empty = true;
    }

    bool is_empty() const noexcept{
        // Can have a race condition in a multi-threaded application
        std::unique_lock<std::mutex> lock(m);
        // if head and tail are equal, we are empty
        return (!this->full && (this->head == this->tail));
    }

    bool is_full() const noexcept{
        // If tail is ahead the head by 1, we are full
        return this->full;
    }

    size_t size() const noexcept{
        // A lock is needed in size ot prevent a race condition, because head_, tail_, and full_
        // can be updated between executing lines within this function in a multi-threaded
        // application
        std::unique_lock<std::mutex> lock(m);
        size_t size = this->capacity();

        if(!this->full){
            if(this->head >= this->tail){
                size = this->head - this->tail;
            }else{
                size = this->capacity() + this->head - this->tail;
            }
        }
        return size;
    }
};

#endif
