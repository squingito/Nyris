
#ifndef MANAGED_THREAD_POOL_H
#define MANAGED_THREAD_POOL_H

#include "ConcurrentPrioQueue.h"
#include <thread>


#define POOL_READY_FL 0X1
#define POOL_KILL 0X2

#define POOL_MASTER_THREAD true
#define POOL_FOLLOWER_THREAD false

#define POOL_RUNNING 1
#define POOL_NOT_RUNNING 0


#include<string>
#include <cstdio>
#include <unistd.h>
#include "ThreadPool.h"


template <typename T, typename Tret> struct ManagedThreadWrap;



template <typename T, typename Tret> void _ManagedThreadRunner(void*);







template <typename T, typename Tret> class ManagedThreadPool:public ThreadPool<void*, void> {
    public:
        ManagedThreadPool(Tret (*processor)(T*), T* state, ConcurrentPrioQueue<T>* in, int64_t size);
        void ManagedThreadPoolInit(Tret (*processor)(T*), T* state, ConcurrentPrioQueue<T>* in, int64_t size);
        ~ManagedThreadPool();
        ConcurrentPrioQueue<T>* getQueue();
        int64_t join();

     

    private:

        PoolFlags flags;
        ConcurrentPrioQueue<T>* queue;
        
        
        

};

template<typename T, typename Tret>
struct ManagedThreadWrap : ThreadWrap<void*, void> {
    funArgs<T*,Tret>* inputFunction;
    ManagedThreadPool<T, Tret>* parent;
    int8_t sigKill;
    bool role;
    std::mutex mux;


    

    ManagedThreadWrap(Tret (*innerFun)(T*), T* state, int16_t idx, bool rol, ManagedThreadPool<T,Tret>* parent) {
        this->type = 1;
        this->index = idx;
        sigKill = 0;
        role = rol;
        thr = new _twrap<void*, void>(_ManagedThreadRunner<T,Tret>, (void*) this);
        inputFunction = new funArgs<T*, Tret>(innerFun, state);
        this->parent = parent;
    }
};

template <typename T, typename Tret>
void _ManagedThreadRunner(void* wrapIn) {
    ManagedThreadWrap<T,Tret>* wrap = (ManagedThreadWrap<T,Tret>*) wrapIn;
    sleep(1);
    
    ConcurrentPrioQueue<T>* queue = wrap->parent->getQueue();
    
    
    while (1) {
        
        wrap->mux.lock();
        if (wrap->sigKill) {
            wrap->mux.unlock();
            return;
        }
        wrap->mux.unlock();
        
        T* val;

        if (queue->pop(&val) >= 0) {
            wrap->inputFunction->fun(val);


        }
        


   
    }
    
    
}

template <typename T, typename Tret> ManagedThreadPool<T,Tret>::ManagedThreadPool(Tret (*processor)(T*), T* state, ConcurrentPrioQueue<T>* in, int64_t size) {
    ManagedThreadPoolInit(processor, state, in, size);
}
template <typename T, typename Tret>
void ManagedThreadPool<T,Tret>::ManagedThreadPoolInit(Tret (*processor)(T*), T* state, ConcurrentPrioQueue<T>* in, int64_t size) {
    queue = in;
    flags.cap = size;
    flags.sig = POOL_NOT_RUNNING;
    flags.mode = 0;

    int16_t idx = 0;
    while (idx < size) {
        sleep(1);
        pool.push_back(new ManagedThreadWrap<T,Tret>(processor, state, idx, (idx == 0 ? 1 : 0), this));
        idx++;
    }
    flags.sig = POOL_RUNNING;
    
    return;

}
template <typename T, typename Tret>
ManagedThreadPool<T,Tret>::~ManagedThreadPool() {}
template <typename T, typename Tret> ConcurrentPrioQueue<T>*  ManagedThreadPool<T,Tret>::getQueue() {
    return queue;
}
template <typename T, typename Tret>
int64_t ManagedThreadPool<T,Tret>::join() {
    int16_t idx = 0;
    if (flags.sig != 1) {
        return -1;
    }
    while (idx < flags.cap) {
        pool.at(idx)->thr->thr.join();
        idx++;
    }
    flags.sig = 2;
    return 0;
}




#endif