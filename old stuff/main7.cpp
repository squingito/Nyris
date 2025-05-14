#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

class Semaphore {
public:
    Semaphore(int count = 0) : count(count) {}

    void wait() {
        std::unique_lock<std::mutex> lock(mutex);
        cond.wait(lock, [this]() { return count > 0; });
        --count;
    }

    void signal(int increment = 1) {
        std::lock_guard<std::mutex> lock(mutex);
        count += increment;
        for (int i = 0; i < increment; ++i) {
            cond.notify_one();  // Wake exactly `increment` consumers
        }
    }

private:
    std::mutex mutex;
    std::condition_variable cond;
    int count;
};

// Shared queue and synchronization tools
std::queue<int> dataQueue;
Semaphore producerSem(1); // Allow only 1 producer at a time
Semaphore consumerSem(0); // Initially, consumers wait
std::mutex queueMutex;

// Producer function (adds multiple values)
void producer(int id, int startValue, int numValues) {
    for (int i = 0; i < 10000; i++) {
    producerSem.wait(); // Ensure only one producer at a time

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::cout << "Producer " << id << " adding " << numValues << " values.\n";

        for (int i = 0; i < numValues; ++i) {
            dataQueue.push(startValue + i);
            std::cout << "Producer " << id << " added value: " << (startValue + i) << std::endl;
        }
    }

    producerSem.signal();   // Allow next producer to proceed
    consumerSem.signal(numValues); // Wake exactly `numValues` consumers
    }
    std::cout << "High prio done\n";
}

// Consumer function
void consumer(int id) {
    for (int i = 0; i < 10000; i++) {
    consumerSem.wait(); // Wait for an available item in the queue

    std::lock_guard<std::mutex> lock(queueMutex);
    if (!dataQueue.empty()) {
        int value = dataQueue.front();
        dataQueue.pop();
        std::cout << "Consumer " << id << " processed value: " << value << std::endl;
    } else {
        std::cout << "Consumer " << id << " woke up but found the queue empty!" << std::endl;
    }
    }
    std::cout << "low prio done\n";
}

int main() {
    std::thread consumers[2], producers[2];

    // Start producers (each adding multiple values)
    producers[0] = std::thread(producer, 1, 10, 1); // Producer 1 adds values 10, 11
    producers[1] = std::thread(producer, 2, 20, 1); // Producer 2 adds values 20, 21, 22

    // Start consumers
    for (int i = 0; i < 2; ++i) {
        consumers[i] = std::thread(consumer, i + 1);
    }

    // Join all threads
    for (auto& t : producers) {
        t.join();
    }
    for (auto& t : consumers) {
        t.join();
    }

    return 0;
}
