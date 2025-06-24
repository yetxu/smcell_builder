#ifndef FIFO_QUEUE_H_
#define FIFO_QUEUE_H_
#include<stdint.h>
#include"mutex.h"
using namespace Mutex;

#define MAX_PACK_SIZE 65535
#define INIT_BUF_SIZE 1000000
#define END_FLAG 0x0000
#define PACK_LEN_SIZE 2

//according to result of experiment, add mutex lock will not casue performance loss compared to without lock, so use lock defaultly
#define USE_LOCK

class FifoQueue{
public:
    FifoQueue(int buf_size = INIT_BUF_SIZE);
    ~FifoQueue();

    void Reset();

    uint16_t PeekNextPackLen();

    //push a complete packet into buffer
    bool PushPacket(char* packet, uint16_t len);

    //pop a complete packet from buffer
    bool PopPacket(char* pacet, uint16_t * len);
private:
    char* buffer_;
    char* buf_end_;
    volatile char* read_pos_;
    volatile char* write_pos_;
    volatile uint64_t read_bytes_;
    volatile uint64_t write_bytes_;
    
    int buf_size_;
#ifdef USE_LOCK
    MutexLock mutex_lock_;
    Condition condition_;
#endif

};
#endif
