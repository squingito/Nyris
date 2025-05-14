#include "ConcurrentPrioQueue.h"

#include <cstdio>
#include <thread>
#include <unistd.h>

struct request {
    int64_t prio;
    int64_t num;
    bool operator<(const request& two) {
        return prio > two.prio;
    } 
};




ConcurrentPrioQueue<request> q;


void lowPrio() {
    int i = 100000;
    while (i != 0) {
        i--;
        request* req;
        

        q.pop((&req));
        if(req->num % 100 == 0) printf("%d\n", req->num);
        delete req;
        
    }
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    printf("lowPrio finished %u\n", milliseconds);
    fflush(stdout);
}

void highPrio() {
    int i = 200000;
    while (i != 0) {
        
        request* req = new request();
        req->num = 200000 -i;
        req->prio =  200000 -i;
        q.push(req);
        i--;
    }
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    printf("HighPrio finished %u\n", milliseconds);
    fflush(stdout);
}

int main() {
    printf("threads: %d\n", std::thread::hardware_concurrency());
    std::thread t1(lowPrio);
    std::thread t2(lowPrio);
    std::thread t3(highPrio);
    printf("here\n");
    t1.join();
    t2.join();
    t3.join();
}