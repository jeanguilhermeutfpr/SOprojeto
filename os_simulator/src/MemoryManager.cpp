#include "MemoryManager.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <iostream>
MemoryManager::MemoryManager(int physMB, int virtMB, PagePolicy pol)
    : physicalFrames(physMB / 4),
      virtualFrames(virtMB  / 4),
      policy(pol)
{
    pageSize = 4;
    frames.resize(physicalFrames);
    lruTime.resize(physicalFrames, 0);
}

// ── Alocar páginas para um processo (alocação preguiços)
bool MemoryManager::allocateProcess(Process& proc) {
    proc.numPages = (int)std::ceil((double)proc.memoryNeeded / pageSize);
    proc.pageTable.assign(proc.numPages, -1); // todas as páginas em disco
    return true;
}

// ── Liberar todos os frames de um processo
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
//conta quantos frames da RAM estão atualmente ocupados e retorna esse número.
int MemoryManager::usedFrames() const {
    int usado = 0;
    for (auto& f : frames) if (!f.free) ++usado;
    return usado;
}
//converte a política escolhida no menu (que é um código interno) para um Texto legível ("FIFO", "LRU" ou "Ótimo") para mostrar na interface gráfica.
std::string MemoryManager::policyName() const {
    switch (policy) {
        case PagePolicy::FIFO:    return "FIFO";
        case PagePolicy::LRU:     return "LRU";
        case PagePolicy::OPTIMAL: return "Ótimo";
    }
    return "";
}

//arre a lista de frames. O primeiro que tiver free == true, ele retorna o número (índice) desse espaço. Se a memória estiver totalmente cheia, ele retorna -1.
int MemoryManager::findFreeFrame() {
    for (int i = 0; i < (int)frames.size(); ++i)
        if (frames[i].free) return i;
    return -1;
}

// ── Políticas de substituição
//fifoQueue (uma fila de quem chegou primeiro), pega o frame que está na frente da fila (o mais antigo), remove-o da fila
int MemoryManager::evictFIFO() {
    if (fifoQueue.empty()) return 0;
    int fi = fifoQueue.front();
    fifoQueue.pop_front();
    std::cout << "      [DEBUG-FIFO] Expulsando frame " << fi << " (era o mais antigo na fila).\n";
    return fi;
}
//Esta lista guarda o momento exato em que cada frame foi acedido pela 
//última vez. O frame que tiver o número menor (ou seja, o relógio mais antigo/que não é usado há mais tempo) é o expulso.
int MemoryManager::evictLRU() {
    int maisAntigo = 0;
    
   
    for (int i = 1; i < (int)lruTime.size(); ++i) {
        if (lruTime[i] < lruTime[maisAntigo]) {
            maisAntigo = i;
        }
    }

   
    std::cout << "      [DEBUG-LRU] Expulsando frame " << maisAntigo 
              << " (Ultimo acesso no relogio: " << lruTime[maisAntigo] << ", Relogio atual: " << clock << ").\n";
              
    return maisAntigo;
}
//Ele olha para a lista acessosFuturos (que o Scheduler.cpp enviou) e procura quando é que cada página que está na RAM vai ser usada no futuro.
// A página que for demorar mais tempo para ser usada novamente (ou que nunca mais for usada, INT_MAX) é a escolhida para ser expulsa.
int MemoryManager::evictOptimal(const std::vector<int>& acessosFuturos) {
    int vitima  = -1;
    int maislonge = -1;
    
    for (int i = 0; i < (int)frames.size(); ++i) {
        if (frames[i].free) return i; // Se achar um livre, usa ele
        
        int proximo = INT_MAX;
        
    
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

  
    int escolhido = (vitima == -1) ? 0 : vitima;

   
    std::cout << "      [DEBUG-OPTIMAL] Expulsando frame " << escolhido;
    if (maislonge == INT_MAX) {
        std::cout << " (Pagina nunca mais sera usada).\n";
    } else {
        std::cout << " (Sera a que vai demorar mais: daqui a " << maislonge << " acessos).\n";
    }
    
    return escolhido;
}
//o que você escolheu no menu do simulador, ele chama uma das três funções acima.
int MemoryManager::evictFrame(const std::vector<int>& acessosFuturos) {
    switch (policy) {
        case PagePolicy::FIFO:    return evictFIFO();
        case PagePolicy::LRU:     return evictLRU();
        case PagePolicy::OPTIMAL: return evictOptimal(acessosFuturos);
    }
    return 0;
}

// ── Acessar página (pode causar falta de página) 
int MemoryManager::accessPage(Process& proc, int pagina, int tempoAtual,
                               const std::vector<int>& acessosFuturos,
                               std::vector<Process>& todosProcessos) { // <-- Nova assinatura
    ++clock;
    if (pagina < 0 || pagina >= proc.numPages) return -1;

    // LOG: Mostra qual processo está tentando acessar qual página e em qual tempo
    std::cout << "[T=" << tempoAtual << "] P" << proc.pid << " acessando pagina " << pagina << " -> ";

    int frameIdx = proc.pageTable[pagina];
    if (frameIdx != -1 && !frames[frameIdx].free) {
        // Acerto de página (page hit)
        std::cout << "HIT! (Ja estava no frame " << frameIdx << ")\n";
        lruTime[frameIdx] = clock;
        return frameIdx;
    }

    // Falta de página (page fault)
    ++pageFaults;
    std::cout << "FAULT! ";
    
    int fi = findFreeFrame();
    
    if (fi == -1) { // Memória cheia, precisa substituir
        std::cout << "Memoria cheia! Acionando politica...\n";
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
    } else {
        // LOG: Achou um frame que já estava vazio
        std::cout << "Alocado no frame livre " << fi << ".\n";
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