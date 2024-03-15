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

    void HarnessPrepare(bool includePerfCounters) {
        includePerf = includePerfCounters;
        results_.clear();
    }

    void HarnessBegin(map<string, double> statistics) {
        begin_vm_statistics_ = statistics;
        if (includePerf) {
            perf_events_.Enable();
            begin_perf_results_ = perf_events_.ReadAll();
        }
        begin_time_ = chrono::steady_clock::now();
    }

    void HarnessEnd(map<string, double> statistics) {
        auto end_time = chrono::steady_clock::now();
        array<uint64_t, N> end_perf_results;
        if (includePerf) {
            end_perf_results = perf_events_.ReadAll();
        }
        for (auto& p : statistics) {
            SetResult(p.first, p.second - begin_vm_statistics_[p.first]);
        }
        SetResult("time", chrono::duration_cast<chrono::milliseconds>(end_time - begin_time_).count());
        if (includePerf) {
            for (size_t i = 0; i < perf_events_.num_events; i++) {
                SetResult(perf_events_.names[i], end_perf_results[i] - begin_perf_results_[i]);
            }
            perf_events_.Disable();
        }
        CalculateResults();
       //Print();
    }

    void STWBegin() {
        if (includePerf) {
            stw_begin_perf_results_ = perf_events_.ReadAll();
        }
    }
    void STWEnd() {
        if (includePerf) {
            auto stw_end_perf_results_ = perf_events_.ReadAll();
            for (size_t i = 0; i < perf_events_.num_events; i++) {
                AddResult(perf_events_.names[i]+".stw", stw_end_perf_results_[i] - stw_begin_perf_results_[i]);
            }
        }
    }

    char** GetResultKeys() {
        return keys_ptr;
    }

    double* GetResultValues() {
        return values_ptr;
    }

    int GetResultResult() {
        return ptr_size;
    }

    char* GetResultAsString() {
        return (char*)result_as_string.c_str();
    }
    
private:
    static constexpr size_t N = 2;
    PerfEvents<N> perf_events_ {
        "PERF_COUNT_SW_TASK_CLOCK"s,
        "PERF_COUNT_HW_CPU_CYCLES"s,
    };
    array<uint64_t, N> begin_perf_results_;
    array<uint64_t, N> stw_begin_perf_results_;
    bool includePerf = false;
    chrono::steady_clock::time_point begin_time_;
    map<string, double> begin_vm_statistics_;

    map<string, double> results_;
    
    // Workarround to returnt he results back to the caller,
    // if the caller's logs don't show messages printed here.
    char** keys_ptr;
    double* values_ptr;
    int ptr_size;
    string result_as_string;

    void SetResult(const string& key, double value) {
        results_[key] = value;
    }

    void AddResult(const string& key, double value) {
        results_[key] += value;
    }

    string ToString(double value) {
        stringstream ss;
        ss << fixed << setprecision(2) << value;
        return ss.str();
    }


    void CalculateResults() {
        if (results_.find("time") != results_.end() && results_.find("time.stw") != results_.end()) {
            results_["time.other"] = results_["time"] - results_["time.stw"];
        }
        for (size_t i = 0; i < perf_events_.num_events; i++) {
            if (results_.find(perf_events_.names[i]) != results_.end() && results_.find(perf_events_.names[i] + ".stw") != results_.end()) {
                results_[perf_events_.names[i] + ".other"] = results_[perf_events_.names[i]] - results_[perf_events_.names[i] + ".stw"];
            }
        }

        result_as_string = "============================ MMTk Statistics Totals ============================\n"; 
        for (auto& x : results_) result_as_string +=  x.first + TAB;
        result_as_string += "\n";
        for (auto& x : results_) result_as_string +=  ToString(x.second) + TAB;
        result_as_string += "\n";
        result_as_string += "Total time: " + ToString(results_.find("time") == results_.end() ? 0 : results_["time"]) + " ms" +"\n";
        result_as_string += "------------------------------ End MMTk Statistics -----------------------------\n" ;
    }

    void Print() {
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
    void harness_prepare(bool includePerfCounters) {
        harness.HarnessPrepare(includePerfCounters);
    }

    void harness_begin(int gcCount, double gcTime) {
        map<string, double> values; 
        values["GC"] = gcCount;
        values["time.stw"] = gcTime;
        harness.HarnessBegin(values);
        
    } 

    void harness_end(int gcCount, double gcTime) {
        map<string, double> values; 
        values["GC"] = gcCount;
        values["time.stw"] = gcTime;
        harness.HarnessEnd(values);
    }

    void harness_stw_begin() {
        harness.STWBegin();
    }
    
    void harness_stw_end() {
        harness.STWEnd();
    }

    char* harness_result_as_string() {
        return harness.GetResultAsString();
    }
}
