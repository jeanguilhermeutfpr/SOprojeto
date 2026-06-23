#pragma once
#include "Process.h"
#include <vector>
#include <string>

class CSVParser {
public:
    // Lê o arquivo CSV e retorna a lista de processos.
    // Em caso de erro, preenche 'erro' com a mensagem e retorna vetor vazio.
    static std::vector<Process> parse(const std::string& caminho, std::string& erro);
};
