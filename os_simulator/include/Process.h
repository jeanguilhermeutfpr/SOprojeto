#pragma once
#include <vector>

enum class ProcessState { NEW, READY, RUNNING, WAITING, TERMINATED };

struct Process {
    // Dados de entrada (CSV)
    int pid;
    int arrivalTime;
    int burstTime;
    int priority;
    int memoryNeeded;

    // Controle do escalonador
    ProcessState state         = ProcessState::NEW;
    int          remainingTime = 0;
    int          startTime     = -1;
    int          finishTime    = 0;

    // Métricas
    int waitTime     = 0;
    int responseTime = -1;

    // Gerência de memória
    int              numPages = 0;
    std::vector<int> pageTable; // pageTable[i] = frame (-1 = em disco)

    Process(int id, int arrival, int burst, int prio, int mem)
        : pid(id), arrivalTime(arrival), burstTime(burst),
          priority(prio), memoryNeeded(mem), remainingTime(burst) {}
};
