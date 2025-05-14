



template <typename T, typename Tret>
ThreadPool<T, Tret>::ThreadPool(Tret (*fun)(T), T state, int16_t size) {
    ThreadPoolInit(fun, state, size);
}

template <typename T, typename Tret>
void ThreadPool<T, Tret>::ThreadPoolInit(T (*fun)(T), T state, int16_t size) {
    flags.cap = size;
    flags.sig = 0;
    flags.mode = 0;

    int16_t idx = 0;
    while (idx < size) {
        pool.push_back(new ThreadWrap(fun, state, idx));
        idx++;
    }
    flags.sig = 1;


}

template <typename T, typename Tret>
int64_t ThreadPool<T,Tret>::join() {
    int16_t idx = 0;
    if (flags.sig != 1) {
        return -1;
    }
    while (idx < flags.cap) {
        pool.at(idx)->thr.join();
        idx++;
    }
    flags.sig = 2;
    return 0;
}

template <typename T, typename Tret>
ThreadPool<T,Tret>::~ThreadPool() {
    join();
    for (int i = pool.size() - 1; i >= 0 ; i--) {
        delete pool.at(i);
        pool.pop_back();
    }
}