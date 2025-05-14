
#ifndef MANAGED_THREAD_POOL_H
#define MANAGED_THREAD_POOL_H

#include "ConcurrentPrioQueue.h"
#include <thread>


#define POOL_READY_FL 0X1
#define POOL_KILL 0X2

#define POOL_MASTER_THREAD true
#define POOL_FOLLOWER_THREAD false

#include<string>
#include <cstdio>
#include <unistd.h>



class ManagedThreadPool;
struct ManagedThreadWrap;




struct PoolFlags {
    int64_t cap:16;
    int64_t sig:4;
    int64_t mode:2;
    
    

};


void _ManagedThreadRunner(ManagedThreadWrap*, ManagedThreadPool*);

struct Request {
    std::string value;
    int64_t prio;

    bool operator<(const Request& two) {
        return prio < two.prio;
    } 

};

struct _twrap {
    std::thread thr;
    _twrap(ManagedThreadWrap* inWrap, ManagedThreadPool* inParent) : thr(_ManagedThreadRunner, inWrap, inParent) {};
};

struct ManagedThreadWrap {
    _twrap* thr;
    std::mutex mux;
    int16_t index:16;
    int8_t type:8;
    int8_t sigKill;
    bool role;
    void* (*innerFunction)(Request* toProcess, void* state);
    void* interState;

    

    ManagedThreadWrap(void* (*innerFun)(Request*, void*), void* state, int16_t idx, bool rol, ManagedThreadPool* parent) {
        innerFunction = innerFun;
        interState = state;
        type = 1;
        index = idx;
        sigKill = 0;
        role = rol;
        thr = new _twrap(this, parent);
    }
};




class ManagedThreadPool {
    public:
        ManagedThreadPool(void* (*processor)(Request*, void*), void* state, ConcurrentPrioQueue<Request>* in, int16_t size) {
            ManagedThreadPoolInit(processor, state, in, size);
        }
        void ManagedThreadPoolInit(void* (*processor)(Request*, void*), void* state, ConcurrentPrioQueue<Request>* in, int16_t size) {
            queue = in;
            flags.cap = size;
            flags.sig = 0;
            flags.mode = 0;

            int16_t idx = 0;
            while (idx < size) {
                
                pool.push_back(new ManagedThreadWrap(processor, state, idx, (idx == 0 ? 1 : 0), this));
                idx++;
            }
            flags.sig = 1;
            
            return;
    
        }
        ~ManagedThreadPool() {}
        ConcurrentPrioQueue<Request>* getQueue() {
            return queue;
        }
        int64_t join() {
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
        

     

    private:

        PoolFlags flags;
        ConcurrentPrioQueue<Request>* queue;
        std::vector<ManagedThreadWrap*> pool;
        
        

};



void _ManagedThreadRunner(ManagedThreadWrap* wrap, ManagedThreadPool* parent) {
    ConcurrentPrioQueue<Request>* queue = parent->getQueue();
    
    
    while (1) {
        
        wrap->mux.lock();
        if (wrap->sigKill) {
            wrap->mux.unlock();
            return;
        }
        wrap->mux.unlock();
        
        Request* val;

        queue->pop(&val);
        
        wrap->innerFunction(val, wrap->interState);
   
    }
    
    
}


#endif