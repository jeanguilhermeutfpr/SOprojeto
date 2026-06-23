#include "Scheduler.h"
#include <algorithm>
#include <deque>
#include <climits>




// ── Auxiliar: adiciona/mescla entradas no Gantt ───────────────────────────────
void Scheduler::addGantt(std::vector<GanttEntry>& gantt, int pid, int inicio, int fim) {
    if (inicio >= fim) return;
    if (!gantt.empty() && gantt.back().pid == pid && gantt.back().endTime == inicio)
        gantt.back().endTime = fim;
    else
        gantt.push_back({pid, inicio, fim});
}

// ── Auxiliar: calcula métricas finais ────────────────────────────────────────
static void calcularMetricas(std::vector<Process>& procs,
                              double& mediaEspera, double& mediaResposta) {
    double totalEspera = 0, totalResposta = 0;
    for (auto& p : procs) {
        totalEspera   += p.waitTime;
        totalResposta += (p.responseTime < 0 ? 0 : p.responseTime);
    }
    int n = (int)procs.size();
    mediaEspera   = n ? totalEspera   / n : 0;
    mediaResposta = n ? totalResposta / n : 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Round-Robin
// ─────────────────────────────────────────────────────────────────────────────
SimulationResult Scheduler::runRR(std::vector<Process> procs) {
    std::vector<GanttEntry> gantt;
    int tempo = 0;
    int n     = (int)procs.size();
    int feitos = 0;
    std::deque<int> filaReady; // índices em procs

    std::sort(procs.begin(), procs.end(),
              [](const Process& a, const Process& b){
                  return a.arrivalTime < b.arrivalTime;
              });

    for (auto& p : procs) mem->allocateProcess(p);

    auto enfileirarChegadas = [&](int t) {
        for (int i = 0; i < n; ++i)
            if (procs[i].arrivalTime == t && procs[i].state == ProcessState::NEW) {
                procs[i].state = ProcessState::READY;
                filaReady.push_back(i);
            }
    };

    auto avancarParaProximo = [&]() {
        int maisProximo = INT_MAX;
        for (auto& p : procs)
            if (p.state == ProcessState::NEW && p.arrivalTime < maisProximo)
                maisProximo = p.arrivalTime;
        if (maisProximo != INT_MAX) {
            addGantt(gantt, -1, tempo, maisProximo);
            tempo = maisProximo;
            enfileirarChegadas(tempo);
        }
    };

    enfileirarChegadas(0);
    if (filaReady.empty()) avancarParaProximo();

    while (feitos < n) {
        if (filaReady.empty()) { avancarParaProximo(); continue; }

        int idx = filaReady.front(); filaReady.pop_front();
        Process& p = procs[idx];
        p.state = ProcessState::RUNNING;

        if (p.responseTime == -1) p.responseTime = tempo - p.arrivalTime;
        if (p.startTime    == -1) p.startTime    = tempo;

        mem->accessPage(p, 0, tempo, {}, procs);

        int fatia = std::min(quantum, p.remainingTime);
        int fimFatia = tempo + fatia;

        for (int t = tempo + 1; t <= fimFatia; ++t) enfileirarChegadas(t);

        addGantt(gantt, p.pid, tempo, fimFatia);
        p.remainingTime -= fatia;
        tempo = fimFatia;

        if (p.remainingTime == 0) {
            p.state      = ProcessState::TERMINATED;
            p.finishTime = tempo;
            p.waitTime   = p.finishTime - p.arrivalTime - p.burstTime;
            mem->freeProcess(p);
            ++feitos;
        } else {
            p.state = ProcessState::READY;
            filaReady.push_back(idx);
        }
    }

    SimulationResult res;
    res.gantt      = gantt;
    res.pageFaults = mem->pageFaults;
    res.processes  = procs;
    calcularMetricas(res.processes, res.avgWaitTime, res.avgResponseTime);
    return res;
}

// ─────────────────────────────────────────────────────────────────────────────
// SJF Preemptivo (Shortest Remaining Time First — SRTF)
// ─────────────────────────────────────────────────────────────────────────────
SimulationResult Scheduler::runSJFP(std::vector<Process> procs) {
    std::vector<GanttEntry> gantt;
    int tempo = 0;
    int n     = (int)procs.size();
    int feitos = 0;

    std::sort(procs.begin(), procs.end(),
              [](const Process& a, const Process& b){
                  return a.arrivalTime < b.arrivalTime;
              });

    for (auto& p : procs) mem->allocateProcess(p);

    // Marcar chegadas no tempo 0
    for (int i = 0; i < n; ++i)
        if (procs[i].arrivalTime == 0) procs[i].state = ProcessState::READY;

    // Selecionar processo com menor tempo restante
    auto selecionarMenor = [&]() -> int {
        int melhor = -1;
        for (int i = 0; i < n; ++i) {
            if (procs[i].state == ProcessState::READY ||
                procs[i].state == ProcessState::RUNNING) {
                if (melhor == -1 ||
                    procs[i].remainingTime < procs[melhor].remainingTime ||
                    (procs[i].remainingTime == procs[melhor].remainingTime &&
                     procs[i].arrivalTime   <  procs[melhor].arrivalTime))
                    melhor = i;
            }
        }
        return melhor;
    };

    while (feitos < n) {
        // Enfileirar chegadas até o tempo atual
        for (int i = 0; i < n; ++i)
            if (procs[i].state == ProcessState::NEW && procs[i].arrivalTime <= tempo)
                procs[i].state = ProcessState::READY;

        int idx = selecionarMenor();
        if (idx == -1) {
            // CPU ociosa — avança até próxima chegada
            int proximo = INT_MAX;
            for (auto& p : procs)
                if (p.state == ProcessState::NEW && p.arrivalTime < proximo)
                    proximo = p.arrivalTime;
            addGantt(gantt, -1, tempo, proximo);
            tempo = proximo;
            continue;
        }

        Process& p = procs[idx];
        p.state = ProcessState::RUNNING;
        if (p.responseTime == -1) p.responseTime = tempo - p.arrivalTime;
        if (p.startTime    == -1) p.startTime    = tempo;

       mem->accessPage(p, 0, tempo, {}, procs);
        // Próximo evento: chegada de um processo que pode causar preempção
        int proximoEvento = INT_MAX;
        for (auto& q : procs)
            if (q.state == ProcessState::NEW && q.arrivalTime > tempo && q.arrivalTime < proximoEvento)
                proximoEvento = q.arrivalTime;

        int rodarAte = (proximoEvento == INT_MAX)
                       ? tempo + p.remainingTime
                       : std::min(tempo + p.remainingTime, proximoEvento);

        addGantt(gantt, p.pid, tempo, rodarAte);
        p.remainingTime -= (rodarAte - tempo);
        tempo = rodarAte;

        // Verificar chegadas
        for (int i = 0; i < n; ++i)
            if (procs[i].state == ProcessState::NEW && procs[i].arrivalTime <= tempo)
                procs[i].state = ProcessState::READY;

        if (p.remainingTime == 0) {
            p.state      = ProcessState::TERMINATED;
            p.finishTime = tempo;
            p.waitTime   = p.finishTime - p.arrivalTime - p.burstTime;
            mem->freeProcess(p);
            ++feitos;
        } else {
            p.state = ProcessState::READY;
        }
    }

    SimulationResult res;
    res.gantt      = gantt;
    res.pageFaults = mem->pageFaults;
    res.processes  = procs;
    calcularMetricas(res.processes, res.avgWaitTime, res.avgResponseTime);
    return res;
}

// ─────────────────────────────────────────────────────────────────────────────
// Prioridade Preemptiva (menor número = maior prioridade)
// ─────────────────────────────────────────────────────────────────────────────
SimulationResult Scheduler::runPriorityP(std::vector<Process> procs) {
    std::vector<GanttEntry> gantt;
    int tempo = 0;
    int n     = (int)procs.size();
    int feitos = 0;

    std::sort(procs.begin(), procs.end(),
              [](const Process& a, const Process& b){
                  return a.arrivalTime < b.arrivalTime;
              });

    for (auto& p : procs) mem->allocateProcess(p);

    for (int i = 0; i < n; ++i)
        if (procs[i].arrivalTime == 0) procs[i].state = ProcessState::READY;

    auto selecionarMaiorPrioridade = [&]() -> int {
        int melhor = -1;
        for (int i = 0; i < n; ++i) {
            if (procs[i].state == ProcessState::READY ||
                procs[i].state == ProcessState::RUNNING) {
                if (melhor == -1 ||
                    procs[i].priority < procs[melhor].priority ||
                    (procs[i].priority == procs[melhor].priority &&
                     procs[i].arrivalTime < procs[melhor].arrivalTime))
                    melhor = i;
            }
        }
        return melhor;
    };

    while (feitos < n) {
        for (int i = 0; i < n; ++i)
            if (procs[i].state == ProcessState::NEW && procs[i].arrivalTime <= tempo)
                procs[i].state = ProcessState::READY;

        int idx = selecionarMaiorPrioridade();
        if (idx == -1) {
            int proximo = INT_MAX;
            for (auto& p : procs)
                if (p.state == ProcessState::NEW && p.arrivalTime < proximo)
                    proximo = p.arrivalTime;
            addGantt(gantt, -1, tempo, proximo);
            tempo = proximo;
            continue;
        }

        Process& p = procs[idx];
        p.state = ProcessState::RUNNING;
        if (p.responseTime == -1) p.responseTime = tempo - p.arrivalTime;
        if (p.startTime    == -1) p.startTime    = tempo;

     mem->accessPage(p, 0, tempo, {}, procs);

        // Executa 1 unidade de tempo (totalmente preemptivo)
        addGantt(gantt, p.pid, tempo, tempo + 1);
        p.remainingTime--;
        tempo++;

        for (int i = 0; i < n; ++i)
            if (procs[i].state == ProcessState::NEW && procs[i].arrivalTime <= tempo)
                procs[i].state = ProcessState::READY;

        if (p.remainingTime == 0) {
            p.state      = ProcessState::TERMINATED;
            p.finishTime = tempo;
            p.waitTime   = p.finishTime - p.arrivalTime - p.burstTime;
            mem->freeProcess(p);
            ++feitos;
        } else {
            p.state = ProcessState::READY;
        }
    }

    SimulationResult res;
    res.gantt      = gantt;
    res.pageFaults = mem->pageFaults;
    res.processes  = procs;
    calcularMetricas(res.processes, res.avgWaitTime, res.avgResponseTime);
    return res;
}

// ── Despacho ──────────────────────────────────────────────────────────────────
SimulationResult Scheduler::run(std::vector<Process> processos) {
    // Resetar estado do gerenciador de memória
    mem->pageFaults = 0;
    mem->clock      = 0;
    mem->frames.assign(mem->frames.size(), Frame{});
    mem->fifoQueue.clear();
    mem->lruTime.assign(mem->lruTime.size(), 0);

    switch (algorithm) {
        case SchedAlgorithm::RR:         return runRR(processos);
        case SchedAlgorithm::SJF_P:      return runSJFP(processos);
        case SchedAlgorithm::PRIORITY_P: return runPriorityP(processos);
    }
    return {};
}
