#include "ThreadPool.h"
#include <cstdio>
#include <iostream>
#include <thread>
#include <unistd.h>

void runner(int64_t in) {
    int64_t i = 10;
    while (i > 0) {
        sleep(1);
        std::cout << "Thread id " << std::this_thread::get_id() << " says " <<  i << "\n";
        i--;
    }
    return;
}

int main() {
    int64_t a = 5;
    ThreadPool<int64_t, void>* tp = new ThreadPool<int64_t, void>(runner, a, 4);
    tp->join();
    return 1;
}