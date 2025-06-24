#include<assert.h>
#include<string.h>
#include<stdio.h>
#include<unistd.h>
#include"fifo_queue.h"

#ifdef USE_LOCK
FifoQueue::FifoQueue(int buf_size):buf_size_(buf_size),
    condition_(mutex_lock_){
    buffer_ = new char[buf_size_];
    buf_end_ = buffer_ + buf_size_;

    read_pos_ = write_pos_ = buffer_;
    read_bytes_ = write_bytes_ = 0;
}
#else
FifoQueue::FifoQueue(int buf_size):buf_size_(buf_size){
    buffer_ = new char[buf_size_];
    buf_end_ = buffer_ + buf_size_;

    read_pos_ = write_pos_ = buffer_;
    read_bytes_ = write_bytes_ = 0;
}
#endif

FifoQueue::~FifoQueue(){
    delete[] buffer_;
}

void FifoQueue::Reset(){
    read_pos_ = write_pos_ = buffer_;
    read_bytes_ = write_bytes_ = 0;
}

bool FifoQueue::PushPacket(char* packet, uint16_t len){
    assert(len < MAX_PACK_SIZE);
    //printf("in push packet fuction, try to lock and push\n");
#ifdef USE_LOCK
    MutexLockGuard lock_guard(mutex_lock_);
#endif
    //printf("in push packet function, before push, write bytes = %ld, read bytes = %ld\n", write_bytes_, read_bytes_);

    //if the space from write_pos_ to buffer_ + buf_size_ is not enough to store len-bytes packet data and 2 bytes length, rewind write_pos_ to buffer_ to continue to write.
    //the limited condition happens when we have len + 1 bytes left from write_pos_ to buffer_ + buf_size_ and also have len + 1 bytes left from buffer_ to read_pos_,
    //in that case, we will reserve maximum space of 2*len + 2 bytes to avoid overwritten the data that has not read
    if (write_bytes_ + 2*len + 2*(PACK_LEN_SIZE - 1) > read_bytes_ + buf_size_){
//        printf("push data faile, buffer overflow!\n");
        return false;
    }
    //if we have only zero or 1 bytes left from write_pos_ to buffer_ + buf_size_, we just rewind the write_pos_, and need not to add end flag
    if (write_pos_ + PACK_LEN_SIZE > buf_end_){
        write_pos_ = buffer_;
    }else if (write_pos_ + len + sizeof(len) > buf_end_){ 
        //insert flag to notify that end of data, and rewind to beginning of buffer to write
        *(uint16_t*)write_pos_ = END_FLAG;
        write_pos_ = buffer_;
    }
    
    *(uint16_t*)write_pos_ = len;
    write_pos_ += PACK_LEN_SIZE;
    memcpy((void*)write_pos_, packet, len);
    write_pos_ += len;
    write_bytes_ += (len + PACK_LEN_SIZE);

#ifdef USE_LOCK
    if (write_bytes_ > read_bytes_){
        condition_.Notify();
    }
#endif
    return true;
}

bool FifoQueue::PopPacket(char* packet, uint16_t* len){
    //printf("in pop packet fuction, try to lock and pop\n");
#ifdef USE_LOCK
    MutexLockGuard lock_guard(mutex_lock_);
    while(read_bytes_ >= write_bytes_){
        condition_.Wait();
    }
#else
    if (read_bytes_ >= write_bytes_){
        return false;
    }
#endif

    if (read_pos_ + PACK_LEN_SIZE > buf_end_){
        read_pos_ = buffer_;
    }
    *len = *(uint16_t*)read_pos_;
    if (*len == END_FLAG){
        read_pos_ = buffer_;
        *len = *(uint16_t*)read_pos_;
    }
    read_pos_ += PACK_LEN_SIZE;
    memcpy(packet,(void*)read_pos_, *len);
    read_pos_ += *len;
    read_bytes_ += (*len + PACK_LEN_SIZE);
    return true;
}

uint16_t FifoQueue::PeekNextPackLen(){
    uint16_t len;

    if (read_bytes_ >= write_bytes_){
//      printf("peek next pack size, read bytes == write_bytes, buffer empty!\n");
        return 0;
    }

    if (read_pos_ + PACK_LEN_SIZE > buf_end_){
        read_pos_ = buffer_;
    }
    len = *(uint16_t*)read_pos_;
    if (len == END_FLAG){
        read_pos_ = buffer_;
        len = *(uint16_t*)read_pos_;
    }
    return len;
}



