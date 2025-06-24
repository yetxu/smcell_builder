#ifndef MUTEX_H_
#define MUTEX_H_
#include<pthread.h>

namespace Mutex{
class MutexLock{
 public:
    MutexLock();
    ~MutexLock();

    void Lock();
    void UnLock();
    pthread_mutex_t* GetPthreadMutex();
 private:
    MutexLock(const MutexLock&);            //for not copyable
    MutexLock& operator=(const MutexLock&); //for not copyable

    pthread_mutex_t mutex_;
};

class MutexLockGuard{
public:
    explicit MutexLockGuard(MutexLock& mutex_lock):
    mutex_lock_(mutex_lock){
        mutex_lock_.Lock();
    }
    ~MutexLockGuard(){
        mutex_lock_.UnLock();
    }
private:
    MutexLockGuard(const MutexLockGuard&);              //for noncopyable
    MutexLockGuard& operator = (const MutexLockGuard&); //for noncopyable

    MutexLock& mutex_lock_;
};

class Condition{
public:
    explicit Condition(MutexLock&);
    ~Condition();

    void Wait();
    int WaitTimeOut(int time); //in second

    void Notify();
    void NotifyAll();

private:
    Condition(const Condition&);
    Condition& operator= (const Condition&);

    MutexLock& mutex_lock_;
    pthread_cond_t pcond_;
};
};
#endif
