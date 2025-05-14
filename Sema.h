
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

class Sema {
public:
    Sema(int64_t);
    Sema();

    void wait();

    void signal(int64_t);
    void signal();

private:
    std::mutex mutex;
    std::condition_variable cond;
    int64_t count;
};