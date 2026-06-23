#include "MemoryManager.h"
#include <algorithm>
#include <climits>
#include <cmath>

MemoryManager::MemoryManager(int physMB, int virtMB, PagePolicy pol)
    : physicalFrames(physMB / 4),
      virtualFrames(virtMB  / 4),
      policy(pol)
{
    pageSize = 4;
    frames.resize(physicalFrames);
    lruTime.resize(physicalFrames, 0);
}

// ── Alocar páginas para um processo (alocação preguiçosa) ─────────────────────
bool MemoryManager::allocateProcess(Process& proc) {
    proc.numPages = (int)std::ceil((double)proc.memoryNeeded / pageSize);
    proc.pageTable.assign(proc.numPages, -1); // todas as páginas em disco
    return true;
}

// ── Liberar todos os frames de um processo ────────────────────────────────────
void MemoryManager::freeProcess(Process& proc) {
    for (int i = 0; i < (int)frames.size(); ++i) {
        if (frames[i].pid == proc.pid) {
            frames[i] = Frame{};
            fifoQueue.erase(
                std::remove(fifoQueue.begin(), fifoQueue.end(), i),
                fifoQueue.end());
        }
    }
    proc.pageTable.assign(proc.numPages, -1);
}

int MemoryManager::usedFrames() const {
    int usado = 0;
    for (auto& f : frames) if (!f.free) ++usado;
    return usado;
}

std::string MemoryManager::policyName() const {
    switch (policy) {
        case PagePolicy::FIFO:    return "FIFO";
        case PagePolicy::LRU:     return "LRU";
        case PagePolicy::OPTIMAL: return "Ótimo";
    }
    return "";
}

// ── Procurar frame livre ──────────────────────────────────────────────────────
int MemoryManager::findFreeFrame() {
    for (int i = 0; i < (int)frames.size(); ++i)
        if (frames[i].free) return i;
    return -1;
}

// ── Políticas de substituição ─────────────────────────────────────────────────

int MemoryManager::evictFIFO() {
    if (fifoQueue.empty()) return 0;
    int fi = fifoQueue.front();
    fifoQueue.pop_front();
    return fi;
}

int MemoryManager::evictLRU() {
    int maisAntigo = 0;
    for (int i = 1; i < (int)lruTime.size(); ++i)
        if (lruTime[i] < lruTime[maisAntigo]) maisAntigo = i;
    return maisAntigo;
}

int MemoryManager::evictOptimal(const std::vector<int>& acessosFuturos) {
    int vitima  = -1;
    int maislonge = -1;
    
    for (int i = 0; i < (int)frames.size(); ++i) {
        if (frames[i].free) return i; // Se achar um livre, usa ele
        
        int proximo = INT_MAX;
        
        // CORREÇÃO: Procuramos quando a PÁGINA LÓGICA (frames[i].page) será usada novamente
        for (int j = 0; j < (int)acessosFuturos.size(); ++j) {
            if (acessosFuturos[j] == frames[i].page) { 
                proximo = j; 
                break; 
            }
        }
        
        if (proximo > maislonge) { 
            maislonge = proximo; 
            vitima = i; 
        }
    }
    return (vitima == -1) ? 0 : vitima;
}

int MemoryManager::evictFrame(const std::vector<int>& acessosFuturos) {
    switch (policy) {
        case PagePolicy::FIFO:    return evictFIFO();
        case PagePolicy::LRU:     return evictLRU();
        case PagePolicy::OPTIMAL: return evictOptimal(acessosFuturos);
    }
    return 0;
}

// ── Acessar página (pode causar falta de página) ──────────────────────────────
int MemoryManager::accessPage(Process& proc, int pagina, int tempoAtual,
                               const std::vector<int>& acessosFuturos,
                               std::vector<Process>& todosProcessos) { // <-- Nova assinatura
    ++clock;
    if (pagina < 0 || pagina >= proc.numPages) return -1;

    int frameIdx = proc.pageTable[pagina];
    if (frameIdx != -1 && !frames[frameIdx].free) {
        // Acerto de página (page hit)
        lruTime[frameIdx] = clock;
        return frameIdx;
    }

    // Falta de página (page fault)
    ++pageFaults;
    int fi = findFreeFrame();
    
    if (fi == -1) { // Memória cheia, precisa substituir
        fi = evictFrame(acessosFuturos);
        
        // CORREÇÃO: Invalida a página na tabela do processo DONO original do frame
        if (!frames[fi].free) {
            int pidDono = frames[fi].pid;
            int paginaRemovida = frames[fi].page;
            
            // Procura quem é o dono na lista geral e atualiza a tabela dele
            for (auto& p : todosProcessos) {
                if (p.pid == pidDono) {
                    p.pageTable[paginaRemovida] = -1;
                    break;
                }
            }
        }
    }

    // Remove da fila FIFO se já estava lá
    fifoQueue.erase(std::remove(fifoQueue.begin(), fifoQueue.end(), fi), fifoQueue.end());

    // Carrega a nova página no frame
    frames[fi] = {proc.pid, pagina, false};
    proc.pageTable[pagina] = fi;
    lruTime[fi] = clock;
    fifoQueue.push_back(fi);

    (void)tempoAtual;
    return fi;
}