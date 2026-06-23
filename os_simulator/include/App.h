#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "Process.h"
#include "Scheduler.h"
#include "MemoryManager.h"

// ── Widgets simples ───────────────────────────────────────────────────────────

struct Botao {
    sf::RectangleShape forma;
    sf::Text           rotulo;
    bool               hover = false;

    Botao(const sf::Font& fonte, const std::string& texto,
          sf::Vector2f pos, sf::Vector2f tam, unsigned tamanhoFonte = 16);

    bool contem(sf::Vector2f p) const { return forma.getGlobalBounds().contains(p); }
    void desenhar(sf::RenderWindow& janela);
    void setHover(bool h);
};

struct CaixaTexto {
    sf::RectangleShape caixa;
    sf::Text           texto;
    sf::Text           placeholder;
    bool               ativo    = false;
    bool               numerico = false;
    
    // ALTERADO: Mudamos para sf::String para dar suporte nativo a acentos
    sf::String         valor;

    CaixaTexto(const sf::Font& fonte, const std::string& dica,
               sf::Vector2f pos, sf::Vector2f tam, unsigned tamanhoFonte = 15);

    bool contem(sf::Vector2f p) const { return caixa.getGlobalBounds().contains(p); }
    void processarCaractere(sf::Uint32 c);
    
    // ADICIONADO: Declarando as novas funções para o compilador não chiar
    void processarTecla(const sf::Event::KeyEvent& key);
    void atualizarTextoVisivel();

    void desenhar(sf::RenderWindow& janela);
};

struct MenuDropdown {
    sf::RectangleShape caixa;
    sf::Text           selecionado;
    std::vector<std::string> opcoes;
    int  atual  = 0;
    bool aberto = false;
    const sf::Font* fonte;
    unsigned tamanhoFonte;

    MenuDropdown(const sf::Font& fonte, std::vector<std::string> opts,
                 sf::Vector2f pos, sf::Vector2f tam, unsigned tamanhoFonte = 15);

    bool contemCabecalho(sf::Vector2f p) const { return caixa.getGlobalBounds().contains(p); }
    int  clicarOpcao(sf::Vector2f p);
    void desenhar(sf::RenderWindow& janela);
};

// ── Telas da aplicação ────────────────────────────────────────────────────────

enum class Tela { CONFIGURACAO, RESULTADOS };

// ── Aplicação principal ───────────────────────────────────────────────────────

class App {
public:
    App();
    void executar();

private:
    sf::RenderWindow janela;
    sf::Font         fonte;
    Tela             telaAtual = Tela::CONFIGURACAO;

    // Widgets da tela de configuração
    CaixaTexto*  tbCSV      = nullptr;
    CaixaTexto*  tbMemFis   = nullptr;
    CaixaTexto*  tbMemVirt  = nullptr;
    CaixaTexto*  tbQuantum  = nullptr;
    MenuDropdown* ddEscal   = nullptr;
    MenuDropdown* ddPagina  = nullptr;
    Botao*       btnIniciar = nullptr;
    Botao*       btnVoltar  = nullptr;

    std::string   msgStatus;
    sf::Color     corStatus;

    // Dados da simulação
    std::vector<Process> processos;
    SimulationResult     resultado;

    // Scroll dos painéis de resultado
    float scrollGantt = 0.f;
    float scrollTabela = 0.f;

    void construirWidgets();

    // Tela de configuração
    void processarEventoConfig(const sf::Event& ev);
    void desenharConfig();

    // Tela de resultados
    void processarEventoResultados(const sf::Event& ev);
    void desenharResultados();

    // Iniciar simulação
    void iniciarSimulacao();

    // Auxiliares de desenho (resultados)
    void desenharGantt(sf::RenderTarget& alvo, sf::FloatRect area);
    void desenharMetricas(sf::RenderTarget& alvo, sf::FloatRect area);
    void desenharTabelaProcessos(sf::RenderTarget& alvo, sf::FloatRect area);
    void desenharGradeMemoria(sf::RenderTarget& alvo, sf::FloatRect area);

    sf::Color corPID(int pid);
    void desenharTexto(sf::RenderTarget& alvo, const std::string& str,
                       sf::Vector2f pos, unsigned tam, sf::Color cor);
};
