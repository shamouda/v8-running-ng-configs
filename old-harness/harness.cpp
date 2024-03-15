#include <iostream>
#include <map>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <iomanip>
#include <sstream>
#include <array>
#include <vector>
#include "perf_event.hpp"

#define TAB '	'

using namespace std;
using namespace std::string_literals;

namespace std {
    string to_string(string s) {
        return s;
    }
}

class Harness {
public:
    void HarnessBegin(map<string, double> statistics) {
        begin_vm_statistics_ = statistics;
        perf_events_.Enable();
        begin_perf_results_ = perf_events_.ReadAll();
        begin_time_ = chrono::steady_clock::now();
    }

    void HarnessEnd(map<string, double> statistics) {
        auto end_time = chrono::steady_clock::now();
        auto end_perf_results = perf_events_.ReadAll();
        for (auto& p : statistics) {
            SetResult(p.first, p.second - begin_vm_statistics_[p.first]);
        }
        SetResult("time", chrono::duration_cast<chrono::milliseconds>(end_time - begin_time_).count());
        for (size_t i = 0; i < perf_events_.num_events; i++) {
            SetResult(perf_events_.names[i], end_perf_results[i] - begin_perf_results_[i]);
        }
        Print();
    }
private:
    static constexpr size_t N = 1;
    PerfEvents<N> perf_events_ {
        "PERF_COUNT_HW_INSTRUCTIONS"s,
        // "PERF_COUNT_HW_CACHE_LL"s,
        // "PERF_COUNT_HW_CACHE_DTLB"s,
        // "PERF_COUNT_HW_CACHE_ITLB"s,
    };
    array<PerfEventData, N> begin_perf_results_;
    chrono::steady_clock::time_point begin_time_;
    map<string, double> begin_vm_statistics_;

    map<string, double> results_;

    void SetResult(const string& key, double value) {
        results_[key] = value;
    }

    string ToString(double value) {
        stringstream ss;
        ss << fixed << setprecision(2) << value;
        return ss.str();
    }

    void Print() {
        if (results_.find("time") != results_.end() && results_.find("time.stw") != results_.end()) {
            results_["time.other"] = results_["time"] - results_["time.stw"];
        }
        cout << "============================ MMTk Statistics Totals ============================" << endl;
        for (auto& x : results_) cout << x.first << TAB;
        cout << endl;
        for (auto& x : results_) cout << ToString(x.second) << TAB;
        cout << endl;
        cout << "Total time: " << ToString(results_.find("time") == results_.end() ? 0 : results_["time"]) << " ms" << endl;
        cout << "------------------------------ End MMTk Statistics -----------------------------" << endl;
    }
};

static Harness harness {};

extern "C" {
    void harness_prepare(size_t num_metrics, const char** metrics) {
        // cout << "harness_prepare\n";
        // va_list valist;
        // vector<string> values;
        // va_start(valist, num_entries);
        // for (size_t i = 0; i < num_entries; i++) {
        //     string key = va_arg(valist, const char*);
        //     double val = va_arg(valist, double);
        //     values[key] = val;
        // }
        // va_end(valist);
    }

    void harness_begin(size_t num_entries, ...) {
        va_list valist;
        map<string, double> values;
        va_start(valist, num_entries);
        for (size_t i = 0; i < num_entries; i++) {
            string key = va_arg(valist, const char*);
            double val = va_arg(valist, double);
            values[key] = val;
        }
        va_end(valist);
        harness.HarnessBegin(values);
    } 

    void harness_end(size_t num_entries, ...) {
        va_list valist;
        map<string, double> values;
        va_start(valist, num_entries);
        for (size_t i = 0; i < num_entries; i++) {
            string key = va_arg(valist, const char*);
            double val = va_arg(valist, double);
            values[key] = val;
        }
        va_end(valist);
        harness.HarnessEnd(values);
    }
}
