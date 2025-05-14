#include <iostream>
#include <queue>
#include <vector>

struct test {
    int64_t prio;
    void* a;
};

struct prioSort {
    bool operator()(const test& one, const test& two) {
        return one.prio > two.prio;
    } 
};

int main() {
    std::priority_queue<test, std::vector<test>, prioSort> queue;
    test a, b, c;
    a.prio = 10;
    b.prio = 1;
    c.prio = 6;
    queue.push(a);
    queue.push(b);
    queue.push(c);
    while (!queue.empty()) {
        test top = queue.top();
        queue.pop();
        printf("%d\n", top.prio);
    }
    return 0;
}