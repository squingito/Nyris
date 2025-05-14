
#include <cstdio>
#include <unistd.h>

template <typename T>
ConcurrentPrioQueue<T>::ConcurrentPrioQueue() {

}

template <typename T>
ConcurrentPrioQueue<T>::~ConcurrentPrioQueue() {}


template <typename T>
int64_t ConcurrentPrioQueue<T>::pop(T** out) {
    
    {

        std::unique_lock<std::mutex> lock(semaTex);
        
        cond.wait(lock, [this]() { return (count > 0 && procCount.load() == 0) || close;});
        count -= 1;
        
    }
    
    if (close) {
        *out = nullptr;
        return -1;
    }

    std::lock_guard<std::mutex> lock2(procLock);
    *out = queue.top();
    queue.pop();
    reqsCompleted++;
    return 0;

}

template <typename T>
int64_t ConcurrentPrioQueue<T>::top(T** out) {
    {
        std::unique_lock<std::mutex> lock(semaTex);
        cond.wait(lock, [this]() { return (count > 0 && procCount.load() == 0) || close;});
    }

    if (close) {
        *out = nullptr;
        return -1;
    }
    std::lock_guard<std::mutex> lock2(procLock);
    *out = queue.top();
    
    return  0;
}

template <typename T>
void ConcurrentPrioQueue<T>::push(T* in) {
    
    procCount.fetch_add(1);
    std::lock_guard<std::mutex> lock(procLock);
    if (in != nullptr) queue.push(in);
    if (queue.size() > maxSize) maxSize = queue.size();

    {
        std::lock_guard<std::mutex> lock2(semaTex);
        count += 1;
        
    }
    procCount.fetch_add(-1);
    cond.notify_one();
    return;
    
}

template <typename T>
void ConcurrentPrioQueue<T>::pushLot(std::vector<T*>* in) {
    
    procCount.fetch_add(1);

    
    int64_t index = 0;

    std::lock_guard<std::mutex> lock(procLock);

    if (in != nullptr) {
        if (queue.size() + in->size() > maxSize) maxSize = queue.size() + in->size();
        while (index != in->size()) {
            queue.push(*(in->at(index)));
            index++;
            
        }
    }

    {
        std::lock_guard<std::mutex> lock2(semaTex);
        if (in != nullptr) count += in->size();
        
    }
    procCount.fetch_add(-1);
    for (int64_t i = 0; i < index; i++) {
        cond.notify_one();
    }
    return;
}

template <typename T>
bool ConcurrentPrioQueue<T>::empty() {
    std::lock_guard<std::mutex> lock(procLock);
    return queue.empty();
}

template <typename T>
int64_t ConcurrentPrioQueue<T>::size() {
    std::lock_guard<std::mutex> lock(procLock);
    return queue.size();
}

