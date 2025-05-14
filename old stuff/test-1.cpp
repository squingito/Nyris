#include "ManagedThreadPool.h"
#include <cstdio>
#include <unistd.h>


std::mutex mux;

struct Request {
    int64_t value;
    int64_t prio;

    bool operator>(const Request& two) const {
        return prio > two.prio;
    } 

};




void toRun(Request* req) {
    printf("Handled req: %ld\n", req->value);
    delete req;
    sleep(1);
    return;
        
    
}

int main(int argc, char const *argv[])
{
    ConcurrentPrioQueue<Request>* reqs = new ConcurrentPrioQueue<Request>();
    int64_t num = 10;
    int64_t num2 = num;
    num = 1;
    while (num < num2) {
        Request* req = new Request();

        req->value = num;
        req->prio = num;
        reqs->push(req);
        
        num = num +1;
        
    }
    //reqs->pop(&req);
    //printf("%d", req->value);

    
    ManagedThreadPool<Request, void>* pool = new ManagedThreadPool<Request,void>(toRun, nullptr, reqs, 4);
    sleep(5);
    

    pool->join();
    delete pool;
    return 0;
    
}
