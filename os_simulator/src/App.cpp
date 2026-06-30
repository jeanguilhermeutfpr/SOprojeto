#include "App.h"
#include "CSVParser.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
//  Paleta de cores 

//sfml
static const sf::Color FUNDO      {240, 244, 248}; // Cinza/Azul bem claro
static const sf::Color PAINEL     {255, 255, 255}; // Branco puro
static const sf::Color DESTAQUE   { 37,  99, 235}; // Azul vibrante
static const sf::Color DESTAQUE2  { 59, 130, 246}; // Azul claro
static const sf::Color SUCESSO    { 34, 197,  94}; // Verde
static const sf::Color PERIGO     {239,  68,  68}; // Vermelho
static const sf::Color TEXTO_PRIN { 15,  23,  42}; // Preto/Azul muito escuro
static const sf::Color TEXTO_DIM  {100, 116, 139}; // Cinza médio
static const sf::Color OCIOSO_COR {203, 213, 225}; // Cinza claro

static const sf::Color CORES_PID[] = {
    {37,99,235}, {236,72,153}, {34,197,94}, {245,158,11},
    {239,68,68}, {6,182,212},  {249,115,22}, {16,185,129},
    {139,92,246},{244,63,94},  {20,184,166},{234,179,8}
};

Botao::Botao(const sf::Font& fonte, const std::string& texto,
             sf::Vector2f pos, sf::Vector2f tam, unsigned tamanhoFonte) {
    forma.setPosition(pos);
    forma.setSize(tam);
    forma.setFillColor(DESTAQUE);

    rotulo.setFont(fonte);
    rotulo.setString(texto);
    rotulo.setCharacterSize(tamanhoFonte);
    rotulo.setFillColor(sf::Color::White);
    auto lb = rotulo.getLocalBounds();
    rotulo.setOrigin(lb.left + lb.width/2.f, lb.top + lb.height/2.f);
    rotulo.setPosition(pos.x + tam.x/2.f, pos.y + tam.y/2.f);
}

void Botao::desenhar(sf::RenderWindow& janela) {
    forma.setFillColor(hover ? sf::Color(120,122,255) : DESTAQUE);
    janela.draw(forma);
    janela.draw(rotulo);
}

void Botao::setHover(bool h) { hover = h; }

//  CaixaTexto 
CaixaTexto::CaixaTexto(const sf::Font& fonte, const std::string& dica,
                       sf::Vector2f pos, sf::Vector2f tam, unsigned tamanhoFonte) {
    caixa.setPosition(pos);
    caixa.setSize(tam);
  caixa.setFillColor(PAINEL);
    caixa.setOutlineColor(sf::Color(200,200,210));
    caixa.setOutlineThickness(1.5f);
    caixa.setOutlineColor(sf::Color(200, 200, 210)); // Borda cinza clara

    texto.setFont(fonte); texto.setCharacterSize(tamanhoFonte);
    texto.setFillColor(TEXTO_PRIN);
    texto.setPosition(pos.x + 8, pos.y + tam.y/2.f - tamanhoFonte/2.f - 1);

    placeholder.setFont(fonte); placeholder.setCharacterSize(tamanhoFonte);
    placeholder.setFillColor(TEXTO_DIM);
    // placeholder suporta sf::String para acentos na dica
    placeholder.setString(sf::String::fromUtf8(dica.begin(), dica.end())); 
    placeholder.setPosition(texto.getPosition());
}

// Método auxiliar para cortar o texto se for maior que a caixa
void CaixaTexto::atualizarTextoVisivel() {
    texto.setString(valor);
    float maxLargura = caixa.getSize().x - 16.f;
    
    // Se o texto for maior que a caixa, cortamos o início para mostrar o final
    if (texto.getLocalBounds().width > maxLargura) {
        sf::String visivel = valor;
        while (texto.getLocalBounds().width > maxLargura && !visivel.isEmpty()) {
            visivel.erase(0, 1);
            texto.setString(visivel);
        }
    }
}

void CaixaTexto::processarCaractere(sf::Uint32 c) {
    if (c == 8) { // Backspace
        if (!valor.isEmpty()) valor.erase(valor.getSize() - 1, 1);
    } 
    // Ignora caracteres de controle ASCII gerados por atalhos (Ctrl+C = 3, Ctrl+V = 22, etc)
    else if (c == 3 || c == 22 || c == 24 || c == 26 || c == 27) {
        return; 
    } 
    else if (c >= 32) {
        if (numerico && (c < '0' || c > '9')) return;
        // Agora aceitamos textos enormes, o limite pode ser maior ou removido
        if (valor.getSize() < 500) valor += c; // sf::String lida com o acento (c) perfeitamente
    }
    atualizarTextoVisivel();
}

// NOVO: Método para lidar com Ctrl+C e Ctrl+V
void CaixaTexto::processarTecla(const sf::Event::KeyEvent& key) {
    if (key.control) {
        if (key.code == sf::Keyboard::C) {
            sf::Clipboard::setString(valor);
        } 
        else if (key.code == sf::Keyboard::V) {
            sf::String areaTransferencia = sf::Clipboard::getString();
            for (auto c : areaTransferencia) {
                if (numerico && (c < '0' || c > '9')) continue;
                if (c >= 32) valor += c;
            }
        }
        atualizarTextoVisivel();
    }
}

void CaixaTexto::desenhar(sf::RenderWindow& janela) {
    caixa.setOutlineColor(ativo ? DESTAQUE : sf::Color(200, 200, 210));
    janela.draw(caixa);
    if (valor.isEmpty() && !ativo) janela.draw(placeholder);
    else janela.draw(texto);
}

//  MenuDropdown 
MenuDropdown::MenuDropdown(const sf::Font& f, std::vector<std::string> opts,
                           sf::Vector2f pos, sf::Vector2f tam, unsigned tf)
    : opcoes(std::move(opts)), fonte(&f), tamanhoFonte(tf) {
    caixa.setPosition(pos); caixa.setSize(tam);
    
    // Fundo branco e borda preta
    caixa.setFillColor(sf::Color::White);
    caixa.setOutlineThickness(1.5f);
    caixa.setOutlineColor(sf::Color::Black);

    selecionado.setFont(f); selecionado.setCharacterSize(tf);
    selecionado.setFillColor(TEXTO_PRIN); // Texto escuro
    selecionado.setString(opcoes[0]);
    selecionado.setPosition(pos.x + 8, pos.y + tam.y/2.f - tf/2.f - 1);
}

int MenuDropdown::clicarOpcao(sf::Vector2f p) {
    if (!aberto) return -1;
    sf::Vector2f base = caixa.getPosition();
    sf::Vector2f sz   = caixa.getSize();
    for (int i = 0; i < (int)opcoes.size(); ++i) {
        sf::FloatRect r(base.x, base.y + sz.y*(i+1), sz.x, sz.y);
        if (r.contains(p)) return i;
    }
    return -1;
}

void MenuDropdown::desenhar(sf::RenderWindow& janela) {
    // A borda fica azul (DESTAQUE) se estiver aberto, senão fica preta
    caixa.setOutlineColor(aberto ? DESTAQUE : sf::Color::Black);
    janela.draw(caixa); janela.draw(selecionado);

    sf::Text seta; seta.setFont(*fonte); seta.setCharacterSize(tamanhoFonte);
    seta.setFillColor(TEXTO_PRIN); seta.setString(aberto ? "▲" : "▼");
    sf::Vector2f bp = caixa.getPosition(), bs = caixa.getSize();
    seta.setPosition(bp.x + bs.x - 24, bp.y + bs.y/2.f - tamanhoFonte/2.f - 1);
    janela.draw(seta);

    if (aberto) {
        for (int i = 0; i < (int)opcoes.size(); ++i) {
            sf::RectangleShape opt({bs.x, bs.y});
            opt.setPosition(bp.x, bp.y + bs.y*(i+1));
            
            // Fundo branco normal, cinza bem clarinho se for o selecionado atual
            opt.setFillColor(i == atual ? sf::Color(240, 240, 245) : sf::Color::White);
            opt.setOutlineThickness(1.0f);
            opt.setOutlineColor(sf::Color::Black); // Margem preta na lista também
            janela.draw(opt);

            sf::Text t; t.setFont(*fonte); t.setCharacterSize(tamanhoFonte);
            t.setFillColor(TEXTO_PRIN); t.setString(opcoes[i]);
            t.setPosition(bp.x + 8, bp.y + bs.y*(i+1) + bs.y/2.f - tamanhoFonte/2.f - 1);
            janela.draw(t);
        }
    }
}

sf::Color App::corPID(int pid) {
    if (pid < 0) return OCIOSO_COR;
    return CORES_PID[(pid - 1) % 12];
}

void App::desenharTexto(sf::RenderTarget& alvo, const std::string& str,
                         sf::Vector2f pos, unsigned tam, sf::Color cor) {
    sf::Text tx;
    tx.setFont(fonte); tx.setString(str);
    tx.setCharacterSize(tam); tx.setFillColor(cor);
    tx.setPosition(pos);
    alvo.draw(tx);
}

App::App()
    : janela(sf::VideoMode(1200, 800),
             "Simulador SO — Escalonamento & Gerencia de Memoria Virtual",
             sf::Style::Default)
{
    janela.setFramerateLimit(60);

  
    if (!fonte.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
        fonte.loadFromFile("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf");

    construirWidgets();
    corStatus = TEXTO_DIM;
}


void App::construirWidgets() {
    float cx = 300.f, larg = 340.f, alt = 38.f, esp = 52.f, inicioY = 165.f;

    tbCSV     = new CaixaTexto(fonte, "Caminho do arquivo CSV...",
                               {cx, inicioY}, {larg, alt});
    tbMemFis  = new CaixaTexto(fonte, "Ex: 256",
                               {cx, inicioY + esp}, {larg/2-5, alt});
    tbMemVirt = new CaixaTexto(fonte, "Ex: 1024",
                               {cx + larg/2+5, inicioY + esp}, {larg/2-5, alt});
    tbQuantum = new CaixaTexto(fonte, "Ex: 4",
                               {cx, inicioY + 2*esp}, {larg/2-5, alt});

    tbMemFis->numerico = tbMemVirt->numerico = tbQuantum->numerico = true;

    ddEscal = new MenuDropdown(fonte,
        {"Round-Robin (RR)", "SJF Preemptivo (SRTF)", "Prioridade Preemptiva"},
        {cx, inicioY + 3*esp}, {larg, alt});

    ddPagina = new MenuDropdown(fonte,
        {"FIFO", "LRU", "Ótimo"},
        {cx, inicioY + 4*esp}, {larg, alt});

    btnIniciar = new Botao(fonte, "Iniciar Simulacao",
                           {cx, inicioY + 5.5f*esp}, {larg, 46.f}, 17);
    btnVoltar  = new Botao(fonte, "Voltar",
                           {30.f, 20.f}, {110.f, 36.f}, 14);
}


void App::processarEventoConfig(const sf::Event& ev) {
    sf::Vector2f mp = janela.mapPixelToCoords(sf::Mouse::getPosition(janela));

    if (ev.type == sf::Event::MouseButtonPressed &&
        ev.mouseButton.button == sf::Mouse::Left) {

        // Ativar/desativar caixas de texto
        auto ativar = [&](CaixaTexto* tb) {
            tbCSV->ativo = tbMemFis->ativo = tbMemVirt->ativo = tbQuantum->ativo = false;
            tb->ativo = true;
        };
        if      (tbCSV->contem(mp))     ativar(tbCSV);
        else if (tbMemFis->contem(mp))  ativar(tbMemFis);
        else if (tbMemVirt->contem(mp)) ativar(tbMemVirt);
        else if (tbQuantum->contem(mp)) ativar(tbQuantum);
        else { tbCSV->ativo=tbMemFis->ativo=tbMemVirt->ativo=tbQuantum->ativo=false; }

        // Dropdowns
        if (ddEscal->contemCabecalho(mp)) {
            ddEscal->aberto = !ddEscal->aberto; ddPagina->aberto = false;
        } else {
            int s = ddEscal->clicarOpcao(mp);
            if (s >= 0) {
                ddEscal->atual = s;
                ddEscal->selecionado.setString(ddEscal->opcoes[s]);
                ddEscal->aberto = false;
            }
        }

        if (ddPagina->contemCabecalho(mp)) {
            ddPagina->aberto = !ddPagina->aberto; ddEscal->aberto = false;
        } else {
            int s = ddPagina->clicarOpcao(mp);
            if (s >= 0) {
                ddPagina->atual = s;
                ddPagina->selecionado.setString(ddPagina->opcoes[s]);
                ddPagina->aberto = false;
            }
        }

        if (btnIniciar->contem(mp)) iniciarSimulacao();
    }

 if (ev.type == sf::Event::TextEntered) {
        if      (tbCSV->ativo)     tbCSV->processarCaractere(ev.text.unicode);
        else if (tbMemFis->ativo)  tbMemFis->processarCaractere(ev.text.unicode);
        else if (tbMemVirt->ativo) tbMemVirt->processarCaractere(ev.text.unicode);
        else if (tbQuantum->ativo) tbQuantum->processarCaractere(ev.text.unicode);
    }

    //COPIAR/COLAR
    if (ev.type == sf::Event::KeyPressed) {
        if      (tbCSV->ativo)     tbCSV->processarTecla(ev.key);
        else if (tbMemFis->ativo)  tbMemFis->processarTecla(ev.key);
        else if (tbMemVirt->ativo) tbMemVirt->processarTecla(ev.key);
        else if (tbQuantum->ativo) tbQuantum->processarTecla(ev.key);
    }
}

void App::desenharConfig() {
    janela.clear(FUNDO);

    desenharTexto(janela, "Simulador de SO", {40, 28}, 30, DESTAQUE);
    desenharTexto(janela, "Escalonamento de Processos & Gerencia de Memoria Virtual",
                  {40, 66}, 15, TEXTO_DIM);

    float cx = 300.f, larg = 340.f, alt = 38.f, esp = 52.f, inicioY = 160.f;

    auto rotulo = [&](const std::string& s, float y, sf::Color c = TEXTO_PRIN) {
        desenharTexto(janela, s, {40.f, y}, 13, c);
    };

    rotulo("Arquivo CSV de Processos",         inicioY );
    rotulo("Memoria Fisica (MB)",              inicioY + esp - 20);
    rotulo("Memoria Virtual (MB)",             inicioY + esp + alt/2 - 20, TEXTO_DIM);
    rotulo("Quantum (unidades de tempo)",      inicioY + 2*esp - 20);
    rotulo("Algoritmo de Escalonamento",       inicioY + 3*esp - 20);
    rotulo("Politica de Substituicao de Pag.", inicioY + 4*esp - 20);

    desenharTexto(janela, "Fisica",  {cx+5,        inicioY+esp-10}, 11, TEXTO_DIM);
    desenharTexto(janela, "Virtual", {cx+larg/2+10, inicioY+esp-10}, 11, TEXTO_DIM);

    tbCSV->desenhar(janela);
    tbMemFis->desenhar(janela);
    tbMemVirt->desenhar(janela);
    tbQuantum->desenhar(janela);
    btnIniciar->desenhar(janela);

 
    ddPagina->desenhar(janela);
    ddEscal->desenhar(janela); 



    desenharTexto(janela, msgStatus, {cx, inicioY + 7.2f*esp}, 13, corStatus);

    // Painel de instruções 
    float rx = 700.f;
    sf::RectangleShape painel({460.f, 560.f});
    painel.setPosition({rx - 10.f, 110.f});
    painel.setFillColor(PAINEL);
    painel.setOutlineThickness(1.f);
    painel.setOutlineColor(sf::Color(55,55,80));
    janela.draw(painel);

    desenharTexto(janela, "Formato do CSV", {rx+5, 120}, 17, DESTAQUE);
    desenharTexto(janela, "4 colunas, sem cabecalho obrigatorio:", {rx+5, 148}, 12, TEXTO_DIM);

    float ry = 172.f;
    std::vector<std::pair<std::string,std::string>> colunas = {
        {"Col 1", "Tempo de Chegada"},
        {"Col 2", "Tempo de Execucao (Burst)"},
        {"Col 3", "Prioridade (menor = mais prioritario)"},
        {"Col 4", "Memoria Necessaria (MB)"},
    };
    for (auto& [k,v] : colunas) {
        desenharTexto(janela, "" + k + "" + v, {rx+12, ry}, 12, TEXTO_PRIN);
        ry += 20.f;
    }

    desenharTexto(janela, "Exemplo de arquivo:", {rx+5, ry+8}, 13, DESTAQUE);
    ry += 30.f;
    sf::RectangleShape bloco({430.f, 110.f});
    bloco.setPosition({rx+5, ry});
    bloco.setFillColor(sf::Color(15,15,25));
    bloco.setOutlineThickness(1.f);
    bloco.setOutlineColor(sf::Color(55,55,80));
    janela.draw(bloco);

    std::vector<std::string> exemplo_csv = {
        "# chegada,burst,prioridade,memoria",
        "0,8,2,64", "1,4,1,32", "2,9,3,128", "4,5,2,48",
    };
    float ey = ry + 8.f;
    for (auto& l : exemplo_csv) {
        desenharTexto(janela, l, {rx+14, ey}, 12,
                      l[0]=='#' ? TEXTO_DIM : sf::Color(150,255,150));
        ey += 18.f;
    }

    ry += 125.f;
    desenharTexto(janela, "Algoritmos:", {rx+5, ry}, 13, DESTAQUE);
    ry += 20.f;
    std::vector<std::string> algos = {
        "Round-Robin: fatia (quantum) configuravel",
        "SJF Preemptivo: menor tempo restante tem prioridade",
        "Prioridade Preemptiva: menor numero = maior prioridade",
    };
    for (auto& a : algos) {
        desenharTexto(janela, a, {rx+10, ry}, 11, TEXTO_PRIN);
        ry += 18.f;
    }

    ry += 8.f;
    desenharTexto(janela, "Substituicao de Paginas:", {rx+5, ry}, 13, DESTAQUE);
    ry += 20.f;
    std::vector<std::string> pols = {
        "FIFO: a primeira pagina carregada e removida primeiro",
        "LRU: a menos recentemente usada e removida",
        "Otimo: substitui a usada mais longe no futuro",
    };
    for (auto& p : pols) {
        desenharTexto(janela, p, {rx+10, ry}, 11, TEXTO_PRIN);
        ry += 18.f;
    }
}


void App::iniciarSimulacao() {
    if (tbCSV->valor.isEmpty()) {
        msgStatus = "Informe o caminho do arquivo CSV.";
        corStatus = PERIGO; return;
    }

  int memFis  = tbMemFis->valor.isEmpty()  ? 256  : std::stoi(tbMemFis->valor.toAnsiString());
    int memVirt = tbMemVirt->valor.isEmpty() ? 1024 : std::stoi(tbMemVirt->valor.toAnsiString());
    int q       = tbQuantum->valor.isEmpty() ? 4    : std::stoi(tbQuantum->valor.toAnsiString());
    if (memFis < 4) {
        msgStatus = "A memoria fisica deve ser de pelo menos 4 MB (1 frame)."; 
        corStatus = PERIGO; 
        return;
    }
  

    if (memFis <= 0 || memVirt <= 0) {
        msgStatus = " Valores de memoria invalidos."; corStatus = PERIGO; return;
    }
    if (memFis > memVirt) {
        msgStatus = " Memoria fisica nao pode ser maior que a virtual.";
        corStatus = PERIGO; return;
    }
    if (memFis <= 0 || memVirt <= 0) {
        msgStatus = " Valores de memoria invalidos."; corStatus = PERIGO; return;
    }
    if (memFis > memVirt) {
        msgStatus = " Memoria fisica nao pode ser maior que a virtual.";
        corStatus = PERIGO; return;
    }
    if (q <= 0) {
        msgStatus = "Quantum deve ser > 0."; corStatus = PERIGO; return;
    }

    std::string erro;
    std::string caminhoCSV = tbCSV->valor.toAnsiString(); 
processos = CSVParser::parse(caminhoCSV, erro);
    if (!erro.empty()) { msgStatus = " " + erro; corStatus = PERIGO; return; }

    PagePolicy pp;
    switch (ddPagina->atual) {
        case 0: pp = PagePolicy::FIFO;    break;
        case 1: pp = PagePolicy::LRU;     break;
        default: pp = PagePolicy::OPTIMAL; break;
    }

    SchedAlgorithm alg;
    switch (ddEscal->atual) {
        case 0: alg = SchedAlgorithm::RR;         break;
        case 1: alg = SchedAlgorithm::SJF_P;      break;
        default: alg = SchedAlgorithm::PRIORITY_P; break;
    }
std::cout << "Opção escolhida no menu: " << ddEscal->atual << std::endl;
    MemoryManager mem(memFis, memVirt, pp);
    Scheduler escalonador(alg, q, &mem);

    resultado    = escalonador.run(processos);
    scrollGantt  = 0.f;
    scrollTabela = 0.f;
    telaAtual    = Tela::RESULTADOS;
    msgStatus    = "";
}


void App::executar() {
    while (janela.isOpen()) {
        sf::Event ev;
        while (janela.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) janela.close();

            if (telaAtual == Tela::CONFIGURACAO)
                processarEventoConfig(ev);
            else
                processarEventoResultados(ev);
        }

        if (telaAtual == Tela::CONFIGURACAO) desenharConfig();
        else                                  desenharResultados();

        janela.display();
    }
}
