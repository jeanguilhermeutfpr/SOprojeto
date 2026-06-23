#include "App.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

// ── Nova Paleta de Cores (Tema Claro/Azul) ────────────────────────────────────
static const sf::Color FUNDO      {240, 244, 248}; // Fundo geral
static const sf::Color PAINEL     {255, 255, 255}; // Fundo branco das caixas
static const sf::Color DESTAQUE   { 37,  99, 235}; // Azul
static const sf::Color TEXTO_PRIN { 15,  23,  42}; // Texto principal escuro
static const sf::Color TEXTO_DIM  {100, 116, 139}; // Texto secundário
static const sf::Color OCIOSO_COR {148, 163, 184}; // Cinza para CPU Ociosa
static const sf::Color SUCESSO    { 34, 197,  94}; // Verde
static const sf::Color PERIGO     {239,  68,  68}; // Vermelho
static const sf::Color BORDA      {200, 200, 210}; // Cinza claro para bordas

static const sf::Color CORES_PID[] = {
    {37,99,235}, {236,72,153}, {34,197,94}, {245,158,11},
    {239,68,68}, {6,182,212},  {249,115,22}, {16,185,129},
    {139,92,246},{244,63,94},  {20,184,166},{234,179,8}
};

//  Formatar número com 2 casas decimais
static std::string fmt(double v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << v;
    return ss.str();
}

//Eventos da tela de resultados 
void App::processarEventoResultados(const sf::Event& ev) {
    sf::Vector2f mp = janela.mapPixelToCoords(sf::Mouse::getPosition(janela));

    if (ev.type == sf::Event::MouseButtonPressed &&
        ev.mouseButton.button == sf::Mouse::Left) {
        if (btnVoltar->contem(mp)) {
            telaAtual = Tela::CONFIGURACAO;
        }
    }

    if (ev.type == sf::Event::MouseWheelScrolled) {
        float delta = ev.mouseWheelScroll.delta * 30.f;
        // Scroll horizontal no Gantt (parte superior)
        if (ev.mouseWheelScroll.y < 400)
            scrollGantt = std::max(0.f, scrollGantt - delta);
        else
            scrollTabela = std::max(0.f, scrollTabela - delta);
    }

    if (ev.type == sf::Event::MouseMoved)
        btnVoltar->setHover(btnVoltar->contem(mp));
}

//Diagrama de Gantt 
void App::desenharGantt(sf::RenderTarget& alvo, sf::FloatRect area) {
    if (resultado.gantt.empty()) return;

    int tempoTotal = resultado.gantt.back().endTime;
    if (tempoTotal == 0) return;

    float larguraTotal = std::max(area.width, (float)tempoTotal * 28.f);
    float escala = larguraTotal / tempoTotal;

    // Fundo do Gantt (agora branco)
    sf::RectangleShape fundo({area.width, area.height});
    fundo.setPosition(area.left, area.top);
    fundo.setFillColor(PAINEL);
    alvo.draw(fundo);

    float barraY  = area.top + 30.f;
    float barraH  = 40.f;
    float labelY  = barraY + barraH + 4.f;

    for (auto& e : resultado.gantt) {
        float x = area.left + e.startTime * escala - scrollGantt;
        float w = (e.endTime - e.startTime) * escala;

        if (x + w < area.left || x > area.left + area.width) continue;

        // Barra colorida
        sf::RectangleShape barra({w - 1.f, barraH});
        barra.setPosition(x, barraY);
        barra.setFillColor(e.pid < 0 ? OCIOSO_COR : CORES_PID[(e.pid-1) % 12]);
        alvo.draw(barra);

        // Rótulo do PID (Branco para dar contraste com as barras coloridas)
        std::string label = (e.pid < 0) ? "—" : "P" + std::to_string(e.pid);
        desenharTexto(alvo, label, {x + 4.f, barraY + 10.f}, 12, sf::Color::White);

        // Marcas de tempo (Texto Dim)
        desenharTexto(alvo, std::to_string(e.startTime),
                      {x, labelY}, 10, TEXTO_DIM);
    }
    // Tempo final
    {
        float x = area.left + tempoTotal * escala - scrollGantt;
        if (x >= area.left && x <= area.left + area.width)
            desenharTexto(alvo, std::to_string(tempoTotal),
                          {x, labelY}, 10, TEXTO_DIM);
    }

    // Linha de régua (Cinza clara)
    sf::RectangleShape linha({area.width, 1.f});
    linha.setPosition(area.left, barraY - 2.f);
    linha.setFillColor(BORDA);
    alvo.draw(linha);
}

//  Painel de métricas 
void App::desenharMetricas(sf::RenderTarget& alvo, sf::FloatRect area) {
    sf::RectangleShape card({area.width, area.height});
    card.setPosition(area.left, area.top);
    card.setFillColor(PAINEL);
    card.setOutlineThickness(1.f);
    card.setOutlineColor(BORDA); // Borda clara
    alvo.draw(card);

    desenharTexto(alvo, "Metricas de Desempenho",
                  {area.left+10, area.top+8}, 15, DESTAQUE);

    struct Metrica { std::string nome; std::string valor; sf::Color cor; };
    std::vector<Metrica> metricas = {
        {"Tempo Medio de Espera",   fmt(resultado.avgWaitTime)   + " ut", SUCESSO},
        {"Tempo Medio de Resposta", fmt(resultado.avgResponseTime) + " ut", DESTAQUE},
        {"Total de Faltas de Pagina",std::to_string(resultado.pageFaults), PERIGO},
        {"Total de Processos",       std::to_string(resultado.processes.size()), TEXTO_PRIN},
    };

    float y = area.top + 32.f;
    for (auto& m : metricas) {
        desenharTexto(alvo, m.nome + ":", {area.left+12, y}, 12, TEXTO_DIM);
        desenharTexto(alvo, m.valor,      {area.left+12, y+14}, 14, m.cor);
        y += 40.f;
    }
}

//  Tabela de processos 
void App::desenharTabelaProcessos(sf::RenderTarget& alvo, sf::FloatRect area) {
    sf::RectangleShape fundo({area.width, area.height});
    fundo.setPosition(area.left, area.top);
    fundo.setFillColor(PAINEL); // Fundo branco
    alvo.draw(fundo);

    std::vector<std::string> cabecalhos = {
        "PID","Chegada","Burst","Prior.","Mem(MB)",
        "Inicio","Fim","Espera","Resposta","Paginas"
    };
    std::vector<float> larguras = {35,55,45,45,60,50,40,55,60,55};

    float x = area.left + 4.f;
    float y = area.top + 4.f - scrollTabela;

    // Cabeçalho
    float xh = x;
    for (int c = 0; c < (int)cabecalhos.size(); ++c) {
        if (y >= area.top && y < area.top + area.height)
            desenharTexto(alvo, cabecalhos[c], {xh, y}, 11, DESTAQUE);
        xh += larguras[c];
    }
    y += 18.f;

    // Linha separadora
    sf::RectangleShape sep({area.width-8, 1.f});
    sep.setPosition(area.left+4, y - scrollTabela > area.top
                    ? y : area.top + 18);
    sep.setFillColor(BORDA); // Linha cinza clara
    alvo.draw(sep);
    y += 4.f;

    // Linhas dos processos
    for (auto& p : resultado.processes) {
        if (y + 16 < area.top || y > area.top + area.height) { y += 20.f; continue; }

        sf::Color cor = (p.pid < 13) ? CORES_PID[(p.pid-1)%12] : TEXTO_PRIN;

        std::vector<std::string> cels = {
            "P"+std::to_string(p.pid),
            std::to_string(p.arrivalTime),
            std::to_string(p.burstTime),
            std::to_string(p.priority),
            std::to_string(p.memoryNeeded),
            std::to_string(p.startTime),
            std::to_string(p.finishTime),
            std::to_string(p.waitTime),
            std::to_string(p.responseTime),
            std::to_string(p.numPages),
        };

        float xc = x;
        for (int c = 0; c < (int)cels.size(); ++c) {
            if (y >= area.top && y < area.top + area.height)
                desenharTexto(alvo, cels[c], {xc, y}, 11, c==0 ? cor : TEXTO_PRIN);
            xc += larguras[c];
        }
        y += 20.f;
    }
}

// Grade de memória (frames físicos)
void App::desenharGradeMemoria(sf::RenderTarget& alvo, sf::FloatRect area) {
    sf::RectangleShape fundo({area.width, area.height});
    fundo.setPosition(area.left, area.top);
    fundo.setFillColor(PAINEL);
    fundo.setOutlineThickness(1.f);
    fundo.setOutlineColor(BORDA);
    alvo.draw(fundo);

    desenharTexto(alvo, "Frames de Memoria Fisica",
                  {area.left+8, area.top+6}, 13, DESTAQUE);

    // Mostrar status resumido dos frames
    desenharTexto(alvo, "Faltas de pagina: " + std::to_string(resultado.pageFaults),
                  {area.left+8, area.top+26}, 12, PERIGO);

    // Grade simples mostrando páginas por processo
    float celW = 28.f, celH = 22.f;
    float gx = area.left + 8.f, gy = area.top + 48.f;
    int col = 0, maxCols = (int)((area.width - 16) / celW);

    for (auto& p : resultado.processes) {
        sf::Color cor = CORES_PID[(p.pid-1)%12];
        for (int pg = 0; pg < p.numPages; ++pg) {
            if (gy + celH > area.top + area.height) break;
            sf::RectangleShape cel({celW-2, celH-2});
            cel.setPosition(gx, gy);
            // Suaviza a cor para o fundo do frame e mantém a borda escura/vibrante
            cel.setFillColor(sf::Color(cor.r, cor.g, cor.b, 200)); 
            cel.setOutlineThickness(1.f);
            cel.setOutlineColor(cor);
            alvo.draw(cel);
            desenharTexto(alvo, "P"+std::to_string(p.pid), {gx+2, gy+4}, 9, sf::Color::White);
            gx += celW;
            if (++col >= maxCols) { col=0; gx=area.left+8.f; gy+=celH; }
        }
    }
}

//Tela de resultados
void App::desenharResultados() {
    // 1. Limpa o fundo geral (agora azul clarinho/cinza)
    janela.clear(FUNDO); 


    btnVoltar->desenhar(janela);

    float W = 1200.f;

    // ─ Gantt (topo)
    sf::FloatRect areaGantt(10, 55, W-20, 100);
    sf::RectangleShape bgGantt({areaGantt.width, areaGantt.height});
    bgGantt.setPosition(areaGantt.left, areaGantt.top);
    bgGantt.setFillColor(PAINEL); // Branco
    bgGantt.setOutlineThickness(1.f);
    bgGantt.setOutlineColor(BORDA); // Borda clara
    janela.draw(bgGantt);
  
    // ─ Métricas (esquerda, meio) 
    desenharMetricas(janela, {10, 165, 220, 210});

    // ─ Grade de memória (centro, meio)
    desenharGradeMemoria(janela, {240, 165, 430, 210});

    // ─ Legenda de processos (direita, meio) 
    {
        sf::RectangleShape card({280.f, 210.f});
        card.setPosition(680, 165);
        card.setFillColor(PAINEL); // Branco
        card.setOutlineThickness(1.f);
        card.setOutlineColor(BORDA); // Borda clara
        janela.draw(card);
        desenharTexto(janela, "Legenda", {692, 173}, 14, DESTAQUE);

        float ly = 193.f;
        for (auto& p : resultado.processes) {
            sf::RectangleShape quad({12.f, 12.f});
            quad.setPosition(692, ly + 1);
            quad.setFillColor(CORES_PID[(p.pid-1)%12]);
            janela.draw(quad);
            std::string info = "P" + std::to_string(p.pid) +
                               "  burst=" + std::to_string(p.burstTime) +
                               "  prio=" + std::to_string(p.priority);
            desenharTexto(janela, info, {710, ly}, 11, TEXTO_PRIN);
            ly += 18.f;
            if (ly > 360) break;
        }

        // Ícone ocioso
        sf::RectangleShape quad({12.f,12.f});
        quad.setPosition(692, ly+1);
        quad.setFillColor(OCIOSO_COR);
        janela.draw(quad);
        desenharTexto(janela, "CPU ociosa", {710, ly}, 11, TEXTO_DIM);
    }

    // ─ Tabela de processos (parte inferior)
    {
        sf::RectangleShape bgTab({W-20, 200.f});
        bgTab.setPosition(10, 385);
        bgTab.setFillColor(PAINEL); // Branco
        bgTab.setOutlineThickness(1.f);
        bgTab.setOutlineColor(BORDA); // Borda clara
        janela.draw(bgTab);
        desenharTexto(janela, "Tabela de Processos (scroll vertical com mouse)",
                      {18, 389}, 11, TEXTO_DIM);
        desenharTabelaProcessos(janela, {10, 403, W-20, 182});
    }

    //  Rodapé 
    desenharTexto(janela, "ut = unidades de tempo", {10, 592}, 11, TEXTO_DIM);

    
}