#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <numeric>
#include <dlfcn.h>



void do_some_work(size_t iter) {
    std::vector<int> v(409600);
    std::generate(v.begin(), v.end(), std::rand);
    std::sort(v.begin(), v.end());
    auto sum = std::accumulate(v.begin(), v.end(), 0);
    std::cout << "[" << iter << "] size = " << v.size() << " sum = " << sum << std::endl;
}

int main() {
    // Prepare
    std::srand(std::time(nullptr));
    auto handle = dlopen("./libharness.so", RTLD_NOW | RTLD_GLOBAL);
    auto harness_begin = (void (*)(size_t, ...)) dlsym(handle, "harness_begin");
    auto harness_end = (void (*)(size_t, ...)) dlsym(handle, "harness_end");
    // Warmup iterations
    for (size_t i = 0; i < 9; i++) {
        do_some_work(i);
    }
    // Timing iteration
    auto gc_count = 100;
    harness_begin(1, "GC", (double) gc_count);
    do_some_work(10);
    harness_end(1, "GC", (double) gc_count + 100);
    return 0;
}