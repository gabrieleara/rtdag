/**
 * @author Alexandre Amory, ReTiS Lab, Scuola Sant'Anna, Pisa, Italy.
 * @brief base class to implement any inter process/thread communication.
 * @version 0.1
 * @date 2022-07-06
 * 
 * @copyright Copyright (c) 2022
 * 
 * Extend this class to create new task communication methods.
 * 
 */
#ifndef CIRCULAR_COMM_H_
#define CIRCULAR_COMM_H_

#include <array>
#include <string>

// based on https://raw.githubusercontent.com/embeddedartistry/embedded-resources/master/examples/cpp/circular_buffer/circular_buffer.hpp

template<class T, size_t TElemCount>
class circular_comm
{
public:
	circular_comm(const std::string& name_): name(name_) {}

	// copy item into tail. block when full
    virtual void push(const T& item) noexcept = 0;
    // copy the head into item. block when empty
	virtual void pop(T& item) noexcept = 0;
    // head == tail
	virtual void reset() noexcept = 0;
	virtual bool is_empty() const noexcept = 0;
	virtual bool is_full() const noexcept = 0;
	virtual size_t size() const noexcept = 0;
	size_t capacity() const noexcept
	{
		return TElemCount;
	}

protected:
	mutable size_t head = 0;
	mutable size_t tail = 0;
	mutable bool full = false;
    mutable bool empty = true;
    std::string name; // used for named shm
};

#endif
