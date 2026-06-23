#pragma once
#include "Process.h"
#include "MemoryManager.h"
#include <vector>

enum class SchedAlgorithm { RR, SJF_P, PRIORITY_P };

struct GanttEntry {
    int pid;       // -1 = CPU ociosa
    int startTime;
    int endTime;
};

struct SimulationResult {
    std::vector<GanttEntry> gantt;
    std::vector<Process>    processes;
    double avgWaitTime     = 0.0;
    double avgResponseTime = 0.0;
    int    pageFaults      = 0;
};

class Scheduler {
public:
    SchedAlgorithm algorithm;
    int            quantum;
    MemoryManager* mem;

    Scheduler(SchedAlgorithm alg, int q, MemoryManager* m)
        : algorithm(alg), quantum(q), mem(m) {}

    SimulationResult run(std::vector<Process> processos);

private:
    SimulationResult runRR(std::vector<Process> procs);
    SimulationResult runSJFP(std::vector<Process> procs);
    SimulationResult runPriorityP(std::vector<Process> procs);

    void addGantt(std::vector<GanttEntry>& gantt, int pid, int inicio, int fim);
};
