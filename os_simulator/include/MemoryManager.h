#pragma once
#include "Process.h"
#include <vector>
#include <deque>
#include <string>

enum class PagePolicy { FIFO, LRU, OPTIMAL };

struct Frame {
    int  pid  = -1;
    int  page = -1;
    bool free = true;
};

class MemoryManager {
public:
    int physicalFrames;
    int virtualFrames;
    int pageSize   = 4; // MB por página
    int pageFaults = 0;
    int clock      = 0;
    PagePolicy policy;

    std::vector<Frame> frames;
    std::deque<int>    fifoQueue; // índices de frames (para FIFO)
    std::vector<int>   lruTime;   // último acesso por frame (para LRU)

    MemoryManager() = default;
    MemoryManager(int physMB, int virtMB, PagePolicy pol);

    bool allocateProcess(Process& proc);
    void freeProcess(Process& proc);

int accessPage(Process& proc, int pagina, int tempoAtual,
               const std::vector<int>& acessosFuturos,
               std::vector<Process>& todosProcessos);

    int         totalFrames() const { return physicalFrames; }
    int         usedFrames()  const;
    std::string policyName()  const;

private:
    int findFreeFrame();
    int evictFrame(const std::vector<int>& futureAccesses);
    int evictFIFO();
    int evictLRU();
    int evictOptimal(const std::vector<int>& futureAccesses);
};
