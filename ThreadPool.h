#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <thread>
#include <mutex>
#include <vector>


struct PoolFlags {
    int64_t cap:16;
    int64_t sig:4;
    int64_t mode:2;
    
};

template <typename T, typename Tret> struct funArgs {
    Tret (*fun)(T);
    T state;

    funArgs(Tret (*funIn)(T), T stateIn) {
        fun = funIn;
        state = stateIn;
    }
};

template <typename T, typename Tret>
struct _twrap {
    std::thread thr;
    _twrap(Tret (*fun)(T), T inState) : thr(fun, inState) {};
};

template <typename T, typename Tret>
struct ThreadWrap {
    _twrap<T,Tret>* thr;
    int64_t index:16;
    int64_t type:8;
    funArgs<T, Tret>* args;
    
    ThreadWrap() {}
    ThreadWrap(Tret (*fun)(T), T state, int16_t idx) {
        index = idx;
        args = new funArgs<T, Tret>(fun, state);
        type = 1;
        thr = new _twrap<T, Tret>(fun, state);

    }
    
    ~ThreadWrap() {
        delete thr;
        delete args;
    }

};

template <typename T, typename Tret>
class ThreadPool {
    public:

        ThreadPool() {};
        ThreadPool(Tret (*fun)(T), T state, int16_t size);
        ~ThreadPool();
        void ThreadPoolInit(Tret (*fun)(T), T state, int16_t size);
        int64_t join();

    protected:
        std::vector<ThreadWrap<T, Tret>*> pool;
        PoolFlags flags;

};

template <typename T, typename Tret>
ThreadPool<T,Tret>::ThreadPool(Tret (*fun)(T), T state, int16_t size) {
    ThreadPoolInit(fun, state, size);
}

template <typename T, typename Tret>
void ThreadPool<T,Tret>::ThreadPoolInit(Tret (*fun)(T), T state, int16_t size) {
    flags.cap = size;
    flags.sig = 0;
    flags.mode = 0;

    int16_t idx = 0;
    while (idx < size) {
        pool.push_back(new ThreadWrap<T, Tret>(fun, state, idx));
        idx++;
    }
    flags.sig = 1;


}

template <typename T, typename Tret>
int64_t ThreadPool<T, Tret>::join() {
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

template <typename T, typename Tret>
ThreadPool<T, Tret>::~ThreadPool() {
    join();
    for (int i = pool.size() - 1; i >= 0 ; i--) {
        delete pool.at(i);
        pool.pop_back();
    }
}

//#include "ThreadPool.tpp"

#endif