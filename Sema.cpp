#include "Sema.h"

Sema::Sema() {
    count = 0;
}

Sema::Sema(int64_t in) {
    count = in;
}

void Sema::wait() {
    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock, [this]() { return count > 0; });
    --count;
    return;
}

void Sema::signal(int64_t in) {
    std::lock_guard<std::mutex> lock(mutex);
    count += in;
    for (int i = 0; i < in; ++i) {
        cond.notify_one(); 
    }
    return;
}

void Sema::signal() {
    this->signal(1);
    return;
}