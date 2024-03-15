#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <err.h>
#include <string>
#include <array>
#include <limits>
#include <perfmon/pfmlib_perf_event.h>
#include "assert.hpp"
#include <sys/ioctl.h>

struct PerfEventData {
    long long raw_count;
    long long time_enabled;
    long long time_running;
    inline bool IsOverflowed() const {
        return raw_count < 0 || time_enabled < 0 || time_running < 0;
    }
    inline bool IsContended() const {
        return time_enabled == 0;
    }
    inline double operator - (PerfEventData& prev) const {
        if (raw_count == 0 && prev.raw_count == 0)
            return 0;
        if (IsOverflowed() || prev.IsOverflowed() || IsContended() || prev.IsContended())
            return std::numeric_limits<double>::quiet_NaN();
        return (double) (raw_count - prev.raw_count);
    }
};

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
    
    std::array<PerfEventData, N> ReadAll() const {
        std::array<PerfEventData, N> results;
        for (size_t i = 0; i < N; i++) {
            pfm_read(i, &results[i]);
        }
        return results;
    }
private:
    std::array<int, N> perf_event_fds_;
    std::array<struct perf_event_attr, N> perf_event_attrs_;
    bool initialized_ = false;

    void pfm_prepare() {
        const int ret = pfm_initialize();
        ASSERT(ret == PFM_SUCCESS, "error in pfm_initialize: %s", pfm_strerror(ret));
        for(size_t i = 0; i < N; i++) {
            perf_event_attrs_[i].size = sizeof(struct perf_event_attr);
        }
        initialized_ = true;
    }

    void pfm_create(size_t id, const char* event_name) {
        struct perf_event_attr* pe = &perf_event_attrs_[id];
        const int ret = pfm_get_perf_event_encoding(event_name, PFM_PLM3, pe, NULL, NULL);
        ASSERT(ret == PFM_SUCCESS, "error creating event %zu '%s': %s\n", id, event_name, pfm_strerror(ret));
        pe->read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
        pe->disabled = 1;
        pe->inherit = 1;
        perf_event_fds_[id] = perf_event_open(pe, 0, -1, -1, 0);
        ASSERT(perf_event_fds_[id] != -1, "error in perf_event_open for event %zu '%s'", id, event_name);
    }

    void pfm_enable() {
        ASSERT(initialized_, "perf events is not initialized");
        const int ret = prctl(PR_TASK_PERF_EVENTS_ENABLE);
        for(size_t i = 0; i < N; i++) {
            ioctl(perf_event_fds_[i], PERF_EVENT_IOC_RESET, 0);
        }
        ASSERT(ret == 0, "error in prctl(PR_TASK_PERF_EVENTS_ENABLE)");
    }

    void pfm_disable() {
        ASSERT(initialized_, "perf events is not initialized");
        const int ret = prctl(PR_TASK_PERF_EVENTS_DISABLE);
        ASSERT(ret == 0, "error in prctl(PR_TASK_PERF_EVENTS_DISABLE)");
    }

    inline void pfm_read(size_t id, PerfEventData* values) const {
        const size_t expected_bytes = 3 * sizeof(long long);
        const int ret = read(perf_event_fds_[id], values, expected_bytes);
        ASSERT(ret >= 0, "error reading event: %s", strerror(errno));
        ASSERT((size_t) ret == expected_bytes, "read of perf event did not return 3 64-bit values");
        ASSERT(values->time_enabled == values->time_running, "perf event counter was scaled");
    }
};
