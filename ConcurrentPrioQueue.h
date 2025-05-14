

#ifndef REQUEST_QUEUE_H
#define REQUEST_QUEUE_H

#include "Sema.h"
#include <cstdint>
#include <queue>
#include <thread>
#include <vector>
#include <climits>
#include <atomic>

/*
    Important:
    Code Expects T to have a function: bool operator>(const T& name ) const in order to define the prio queue order
*/

template <typename T>
struct DerefLess {
    bool operator()(const T* a, const T* b) const {
        return *a > *b;  // Delegates to T::operator<
    }
};

template <typename T>
class ConcurrentPrioQueue {
    public:
        ConcurrentPrioQueue();
        ~ConcurrentPrioQueue();

        int64_t pop(T**);
        int64_t top(T**);
        bool empty();
        void push(T*);
        int64_t size();
        void pushLot(std::vector<T*>*);


    private:
        std::priority_queue<T*, std::vector<T*>, DerefLess<T>> queue;
        std::mutex procLock;

        int64_t reqsCompleted = 0; 
        int64_t maxSize = 0;
        
        std::mutex semaTex;
        std::condition_variable cond;
        int64_t count = 0;
        std::atomic_int64_t procCount{0};
        bool close = false;

};

#include "ConcurrentPrioQueue.tpp"

#endif