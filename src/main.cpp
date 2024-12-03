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

using boost::asio::ip::tcp;

std::counting_semaphore<> escritores(1);  // Apenas um escritor pode acessar por vez
std::counting_semaphore<> leitores(1);    // Controla o número de leitores ativos
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

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    read_message();
  }

private:
    void ajustarString(std::string& str) {
        str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());  // Remove '\r'
        str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());  // Remove '\n'
    }

    void ajustarstringEnviar(std::string& str) {
        str += "\r\n";
    }

    std::vector<std::string> separarMensagem(const std::string &str, char delimitador) {
        std::vector<std::string> partes;
        std::string parte;
        std::istringstream stream(str);

        while (std::getline(stream, parte, delimitador)) {
            partes.push_back(parte);
        }

        return partes;
    }

    std::string processarAcao(const std::vector<std::string> &tokens) {
        std::string response;
        if (tokens.empty()) {
            response = "Erro: Comando inválido.\n";
            
            return response;
        }

        std::string command = tokens[0];  // Primeira parte é a ação do comando

        if (command == "GET" && tokens.size() == 3) {
            // Esperado: GET <SENSOR_ID> <NUMERO_DE_REGISTROS>
            
            // Ler Dados no arquivo
            // - checar se o arquivo existe
            // - abrir o arquivo
            // - ler arquivo
            // - salvar em uma struct ou vetor
            // - transformar struct ou vetor em string
            // - retornar string
            std::string nomeArq, strNum;
            nomeArq = tokens[1];
            strNum = tokens[2];
            int num = std::stoi(strNum);

            adicionarExtensao(nomeArq);
            if(!checarExistearquivo(nomeArq)){
                return response = "ERROR|INVALID_SENSOR_ID\r\n";
            }

            response = abrirarquivoLeitura(nomeArq, num);

            return response;   
        } 

        else if (command == "LOG" && tokens.size() == 4) {
            // Esperado: LOG <SENSOR_ID> <DATA_HORA> <LEITURA>

            // Escrever Dados no arquivo
            // - checar se o arquivo existe e criá-lo caso não exista
            // - abrir o arquivo
            // - escrever dados no arquivo
            // - informar que os dados foram salvos
            std::string nomeArq;
            nomeArq = tokens[1];

            LogRecord dadoSensor; 

            std::strcpy(dadoSensor.sensor_id, nomeArq.c_str());
            dadoSensor.timestamp = string_to_time_t(tokens[2]);
            dadoSensor.value = std::stod(tokens[3]);

            adicionarExtensao(nomeArq);
            checarExistearquivo(nomeArq);
            if(!checarExistearquivo(nomeArq)){
                criarArquivo(nomeArq);
            }
            
            response = abrirarquivoEscrita(nomeArq, dadoSensor);

            return response;
        } 
        
        else {
            response = R"(
            Erro: Comando inválido ou numero incorreto de parâmetros.
            Formatos válidos:
            LOG|SENSOR_ID|DATA_HORA|LEITURA.
            GET|SENSOR_ID|NUMERO_DE_REGISTROS.)";
        }
        return response;
    }

    std::time_t string_to_time_t(const std::string& time_string) {
        std::tm tm = {};
        std::istringstream ss(time_string);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        return std::mktime(&tm);
    }

    std::string time_t_to_string(std::time_t time) {
        std::tm* tm = std::localtime(&time);
        std::ostringstream ss;
        ss << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");
        return ss.str();
    }

    std::string adicionarExtensao(std::string& filename) {
        const std::string extension = ".dat";
        filename += extension;  // Adiciona a extensão ao nome do arquivo
        std::cout << "Nome do arquivo: " << filename << std::endl;
        return filename;
    }

    std::string juntarStringLeitura(std::vector<DadosArquivo> &vec) {
        int numReg;
        numReg = vec.size();
        std::string novaString;
        novaString = std::to_string(numReg) + ';';

        for (size_t i = 0; i < vec.size(); ++i) {
            novaString += time_t_to_string(vec[i].data); 
            novaString += '|'; 
            novaString += std::to_string(vec[i].valorLeitura);
            novaString += ';';
        }

        vec.clear(); // Limpar vetor de structs

        return novaString;
    }

    std::string juntarStringEscrita(LogRecord sensorData) {
        std::string novaString;

        novaString = "REGISTO ADICIONADO|";
        novaString += sensorData.sensor_id; 
        novaString += '|'; 
        novaString += time_t_to_string(sensorData.timestamp); 
        novaString += '|'; 
        novaString += std::to_string(sensorData.value);

        return novaString;
    }

    bool checarExistearquivo(const std::string& filename) {
        // Conferir se o arquivo existe
        std::ifstream file_check(filename);

        if (!file_check) {
            std::cout << "Arquivo não existe! " << std::endl;
            return false;
        }
        else{
            std::cout << filename << " existe!" << std::endl;
            return true;
        }
    }

    void criarArquivo(const std::string& filename){
        std::ofstream new_file(filename);
        std::cout << "Arquivo: " << filename << "criado com sucesso!" << std::endl;
        return;
    }

    std::string abrirarquivoLeitura(const std::string& filename, int num) {
        // Abrir o arquivo para leitura de dados [em modo append])
        std::fstream file(filename, std::fstream::out | std::fstream::in | std::fstream::binary | std::fstream::app);
        std::string stringResposta;

        std::cout << "Aguardando escritores. " << std::endl;
        leitores.acquire();  // Espera até que não haja escritores
        std::cout << "Semáforo de leitores adquirido." << std::endl;

        {
            std::lock_guard<std::mutex> lock(mtx);
            contadorLeitores++;
            if (contadorLeitores == 1) {
                // Se for o primeiro leitor, bloqueia escritores
                escritores.acquire();
                std::cout << "Primeiro Leitor do arquivo: " << filename << std::endl;
            }
        }


        // Caso não ocorram erros na abertura do arquivo
        if (file.is_open()){
            std::cout << "Arquivo: " << filename << " aberto." << std::endl;
            
            file.seekg(0, std::ios::end);
            
            // Imprime a posição atual do apontador do arquivo (representa o tamanho do arquivo)
            int file_size = file.tellg();
            std::cout << "file_size: " << file_size << std::endl;

            // Recupera o número de registros presentes no arquivo
            int n = file_size/sizeof(DadosArquivo);
            std::cout << "n: " << n << std::endl;

            std::vector<DadosArquivo> dados;
            DadosArquivo dado;  // Para armazenar os dados lidos

            if(n < num){
                // se tem menos registros dos que os solicitados, faz a leitura de todos os que têm
                std::cout << "n < num: " << std::endl;
                file.seekg(0, std::ios_base::beg);
            }
            else if (n >= num) {
                file.seekg((n-num)*sizeof(DadosArquivo), std::ios_base::beg);    
            }

            // Lendo os registros do ponto desejado até o final do arquivo
            while (file.read(reinterpret_cast<char*>(&dado), sizeof(DadosArquivo))) {
                dados.push_back(dado);  // Adiciona o registro lido ao vetor
            }
        
            file.close();  // Fechar o arquivo após a leitura

            
            // Fim da leitura
            {
                std::lock_guard<std::mutex> lock(mtx);
                contadorLeitores--;
                if (contadorLeitores == 0) {
                    // Se for o último leitor, libera a espera de escritores
                    escritores.release();
                }
            }
            leitores.release();  // Libera a permissão para outro leitor entrar
            

            stringResposta = juntarStringLeitura(dados);
        }
        else{
            stringResposta = "Erro ao abrir o arquivo.";
        }

        return stringResposta;
    }

    std::string abrirarquivoEscrita(const std::string& filename, LogRecord sensor) {
        // Abrir o arquivo para escrita de dados [em modo append])
        std::fstream file(filename, std::fstream::out | std::fstream::in | std::fstream::binary | std::fstream::app);
        std::string stringResposta;
        // Caso não ocorram erros na abertura do arquivo

        
        escritores.acquire();  // Espera até que não haja leitores
        

        if (file.is_open()){
            // Imprime a posição atual do apontador do arquivo (representa o tamanho do arquivo)
            int file_size = file.tellg();

            // Recupera o número de registros presentes no arquivo
            int n = file_size/sizeof(LogRecord);

            // Escreve registro no arquivo
            DadosArquivo registro;

            registro.data = sensor.timestamp;
            registro.valorLeitura = sensor.value;
            file.write((char*)&registro, sizeof(DadosArquivo));

            // Fecha o arquivo
            file.close();
            
            
            escritores.release();  // Libera a permissão para leitores
            

            stringResposta = juntarStringEscrita(sensor);
        }
        else
        {
            stringResposta = "Erro ao abrir o arquivo!";
        }
        return stringResposta;
    }

  void read_message()
  {
    auto self(shared_from_this());
    boost::asio::async_read_until(socket_, buffer_, "\r\n",
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            std::istream is(&buffer_);
            std::string message(std::istreambuf_iterator<char>(is), {});
            // - separar a mensagem
            // - identificar o que é para ser feito
            // - fazer o que está sendo solicitado
            // - enviar resposta write_message(resposta)

            ajustarString(message);
            std::cout << "Mensagem após ajuste: " << message << std::endl;

            std::vector<std::string> vetorMensagem;
            vetorMensagem = separarMensagem(message, '|');

            std::cout << "Mensagem separada em tokens: ";
            for (const auto& token : vetorMensagem) {
                std::cout << "[" << token << "] ";  // Verificar cada token
            }
            std::cout << std::endl;

            std::string resultado;
            resultado = processarAcao(vetorMensagem);
            std::cout << "Mensagem após consulta: " << resultado << std::endl;
            ajustarstringEnviar(resultado);
            write_message(resultado);

          }
        });
  }

  void write_message(const std::string& message)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(message),
        [this, self, message](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            read_message();
          }
        });
  }

  tcp::socket socket_;
  boost::asio::streambuf buffer_;
};

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    accept();
  }

private:
  void accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket))->start();
          }

          accept();
        });
  }

  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: chat_server <port>\n";
    return 1;
  }

  boost::asio::io_context io_context;

  server s(io_context, std::atoi(argv[1]));

  io_context.run();

  return 0;
}
