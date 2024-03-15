#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <err.h>
#include <string>
#include <array>
#include <limits>
#include "assert.hpp"
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <unistd.h>
using namespace std;

static int perf_event_open(perf_event_attr* attr, pid_t pid, int cpu, int group_fd,
                           unsigned long flags) {
  return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

void init_event_attr(std::string perf_event_name, perf_event_attr* pe) {
    if (perf_event_name == "PERF_COUNT_SW_TASK_CLOCK") {
        pe->type = PERF_TYPE_SOFTWARE;
        pe->config = PERF_COUNT_SW_TASK_CLOCK;
    } else if (perf_event_name == "PERF_COUNT_HW_CPU_CYCLES") {
        pe->type = PERF_TYPE_HARDWARE;
        pe->config = PERF_COUNT_HW_CPU_CYCLES;
    } else if (perf_event_name == "PERF_COUNT_HW_INSTRUCTIONS") {
        pe->type = PERF_TYPE_HARDWARE;
        pe->config = PERF_COUNT_HW_INSTRUCTIONS;
    } else {
        ASSERT(false, "Unknown performance counter name");
    }
    pe->size = PERF_ATTR_SIZE_VER1;
    pe->exclude_user = 0;
    pe->exclude_kernel = 0;
    pe->exclude_hv = 0;
    pe->read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    pe->disabled = 1;
    pe->inherit = 1;
}

template <size_t N>
class PerfEvents {
public:
    const std::vector<std::string> names;
    const size_t num_events = N;

    PerfEvents(std::initializer_list<std::string> names): names { names } {
        pfm_prepare();
        for (size_t i = 0; i < N; i++) {
            pfm_create(i, this->names[i].c_str());
        }
    }

    void Enable() {
        pfm_enable();
    }

    void Disable() {
        pfm_disable();
    }

    std::array<uint64_t, N> ReadAll() const {
        std::array<uint64_t, N> results;
        for (size_t i = 0; i < N; i++) {
            results[i] = pfm_read(i);
        }
        return results;
    }
private:
    std::array<int, N> perf_event_fds_;
    std::array<struct perf_event_attr, N> perf_event_attrs_;
    bool initialized_ = false;

    void pfm_prepare() {
        for(size_t i = 0; i < N; i++) {
            perf_event_attrs_[i].size = sizeof(struct perf_event_attr);
        }
        initialized_ = true;
    }

    void pfm_create(size_t id, const char* event_name) {
        struct perf_event_attr* pe = &perf_event_attrs_[id];
        init_event_attr(event_name, pe);
        perf_event_fds_[id] = perf_event_open(pe, 0 /* pid */, -1 /* cpu */, -1 /* group_fd */, 0 /* flags */);
        ASSERT(perf_event_fds_[id] != -1, "error in perf_event_open for event %zu '%s'", id, event_name);
    }

    void pfm_enable() {
        ASSERT(initialized_, "perf events is not initialized");
        for(size_t i = 0; i < N; i++) {
            ioctl(perf_event_fds_[i], PERF_EVENT_IOC_RESET, 0);
            ioctl(perf_event_fds_[i], PERF_EVENT_IOC_ENABLE, 0);
        }
    }

    void pfm_disable() {
        ASSERT(initialized_, "perf events is not initialized");
        for(size_t i = 0; i < N; i++) {
            ioctl(perf_event_fds_[i], PERF_EVENT_IOC_DISABLE, 0);
        }
    }

    inline uint64_t pfm_read(size_t id) const {  
        uint64_t values[3];
        memset(values, 0, sizeof(values));
        ssize_t ret = read(perf_event_fds_[id], values, sizeof(values));
        if (ret < (ssize_t) sizeof(values)) {
            cout << "pfm_read error: Read failed for perf counter " << names[id] << endl;
            return 0;
        } else {
            if (values[1] != values[2]) {
                cout << "pfm_read warning: Multiplexed counters for " << names[id] << "\n  "
                    << "Counter total time enabled " << values[1] << "\n  "
                    << "Counter total time running " << values[2] << endl;
                return 0;
            }
            return values[0];
        }
        return 0;
    }
};
