#include "CSVParser.h"
#include <fstream>
#include <sstream>
#include <algorithm>

static std::string trim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        [](unsigned char c){ return !std::isspace(c); }));
    s.erase(std::find_if(s.rbegin(), s.rend(),
        [](unsigned char c){ return !std::isspace(c); }).base(), s.end());
    return s;
}

std::vector<Process> CSVParser::parse(const std::string& caminho, std::string& erro) {
    std::vector<Process> procs;
    erro.clear();

    std::ifstream arq(caminho);
    if (!arq.is_open()) {
        erro = "Não foi possível abrir o arquivo: " + caminho;
        return {};
    }

    std::string linha;
    int numLinha = 0;
    int pid = 1;

    while (std::getline(arq, linha)) {
        ++numLinha;
        linha = trim(linha);
        if (linha.empty() || linha[0] == '#') continue;

        // Ignorar cabeçalho se houver
        if (numLinha == 1) {
            std::string minusc = linha;
            std::transform(minusc.begin(), minusc.end(), minusc.begin(), ::tolower);
            if (minusc.find("chegada") != std::string::npos ||
                minusc.find("arrival") != std::string::npos ||
                minusc.find("burst")   != std::string::npos) continue;
        }

        std::istringstream ss(linha);
        std::string tok;
        std::vector<std::string> tokens;
        while (std::getline(ss, tok, ',')) tokens.push_back(trim(tok));

        if (tokens.size() < 4) {
            erro = "Linha " + std::to_string(numLinha) +
                   " inválida (esperado 4 colunas: chegada,burst,prioridade,memoria).";
            return {};
        }

        try {
            int chegada    = std::stoi(tokens[0]);
            int burst      = std::stoi(tokens[1]);
            int prioridade = std::stoi(tokens[2]);
            int memoria    = std::stoi(tokens[3]);

            if (chegada < 0)  { erro = "Tempo de chegada negativo (linha " + std::to_string(numLinha) + ")."; return {}; }
            if (burst   <= 0) { erro = "Burst deve ser > 0 (linha "        + std::to_string(numLinha) + ")."; return {}; }
            if (memoria <= 0) { erro = "Memória deve ser > 0 (linha "      + std::to_string(numLinha) + ")."; return {}; }

            procs.emplace_back(pid++, chegada, burst, prioridade, memoria);
        } catch (...) {
            erro = "Valor não numérico na linha " + std::to_string(numLinha) + ".";
            return {};
        }
    }

    if (procs.empty()) {
        erro = "O arquivo CSV não contém processos válidos.";
        return {};
    }
    return procs;
}
