#ifndef COMMON_HPP
#define COMMON_HPP
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <semaphore>
#include <condition_variable>
#include <mutex>


std::counting_semaphore<> escritores(1);  // Apenas um escritor pode acessar por vez
std::counting_semaphore<> leitores(0);    // Controla o número de leitores ativos
std::mutex mtx;  // Mutex para proteger a variável de leitura

int contadorLeitores = 0;  // Contador de leitores ativos

struct LogRecord {
    char sensor_id[32]; // supondo um ID de sensor de até 32 caracteres
    std::time_t timestamp; // timestamp UNIX
    double value; // valor da leitura
};

struct DadosArquivo {
    std::time_t data; // timestamp UNIX
    double valorLeitura;
};

void ajustarString(std::string& str);
void ajustarstringEnviar(std::string& str);
std::vector<std::string> separarMensagem(const std::string &str, char delimitador);
std::string processarAcao(const std::vector<std::string> &tokens);
std::time_t string_to_time_t(const std::string& time_string);
std::string time_t_to_string(std::time_t time);
std::string adicionarExtensao(std::string& filename);
std::string juntarStringLeitura(std::vector<DadosArquivo> &vec);
std::string juntarStringEscrita(LogRecord sensorData);
bool checarExistearquivo(const std::string& filename);
void criarArquivo(const std::string& filename);
std::string abrirarquivoLeitura(const std::string& filename, int num);
std::string abrirarquivoEscrita(const std::string& filename, LogRecord sensor);


#endif // COMMON_HPP
