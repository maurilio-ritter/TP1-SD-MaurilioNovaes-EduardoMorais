#include <iostream>
#include <vector>
#include <thread>
#include <semaphore>
#include <mutex>
#include <random>
#include <atomic>
#include <chrono>
#include <fstream>

using namespace std;

constexpr int M = 100000;

int N, Np, Nc;

vector<int> bufferCompartilhado;
int inPos = 0;
int outPos = 0;
int ocupacao = 0;

counting_semaphore<>* livres;
counting_semaphore<>* ocupados;

mutex mtx;
atomic<int> produzidos{0};
atomic<int> consumidos{0};
atomic<bool> terminou{false};

vector<int> historicoOcupacao;

bool isPrime(int n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (int i = 3; 1LL * i * i <= n; i += 2)
        if (n % i == 0) return false;
    return true;
}

void produtor() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(1, 10000000);

    while (true) {
        int id = produzidos.fetch_add(1);

        if (id >= M) {
            produzidos.fetch_sub(1);
            break;
        }

        int numero = dist(gen);

        livres->acquire();

        {
            lock_guard<mutex> lock(mtx);

            bufferCompartilhado[inPos] = numero;
            inPos = (inPos + 1) % N;
            ocupacao++;

            historicoOcupacao.push_back(ocupacao);
        }

        ocupados->release();
    }
}

void consumidor() {
    while (true) {
        if (consumidos.load() >= M) break;

        ocupados->acquire();

        if (consumidos.load() >= M) {
            ocupados->release();
            break;
        }

        int numero;

        {
            lock_guard<mutex> lock(mtx);

            numero = bufferCompartilhado[outPos];
            outPos = (outPos + 1) % N;
            ocupacao--;

            historicoOcupacao.push_back(ocupacao);
        }

        livres->release();

        bool primo = isPrime(numero);

        int atual = consumidos.fetch_add(1) + 1;

        if (atual <= 50) {
            cout << numero << " -> "
                 << (primo ? "primo" : "nao primo")
                 << endl;
        }

        if (atual >= M) {
            terminou = true;
            for (int i = 0; i < Nc; i++)
                ocupados->release();
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Uso: " << argv[0] << " <N> <Np> <Nc>\n";
        return 1;
    }

    N = stoi(argv[1]);
    Np = stoi(argv[2]);
    Nc = stoi(argv[3]);

    bufferCompartilhado.resize(N);

    livres = new counting_semaphore<>(N);
    ocupados = new counting_semaphore<>(0);

    vector<thread> produtores;
    vector<thread> consumidores;

    auto inicio = chrono::high_resolution_clock::now();

    for (int i = 0; i < Np; i++)
        produtores.emplace_back(produtor);

    for (int i = 0; i < Nc; i++)
        consumidores.emplace_back(consumidor);

    for (auto& t : produtores)
        t.join();

    for (auto& t : consumidores)
        t.join();

    auto fim = chrono::high_resolution_clock::now();

    chrono::duration<double> tempo = fim - inicio;

    cout << "Tempo de execucao: " << tempo.count() << " segundos\n";

    string nomeArquivo = "ocupacao_N" + to_string(N) +
                         "_P" + to_string(Np) +
                         "_C" + to_string(Nc) + ".csv";

    ofstream arquivo(nomeArquivo);
    arquivo << "operacao,ocupacao\n";

    for (size_t i = 0; i < historicoOcupacao.size(); i++)
        arquivo << i << "," << historicoOcupacao[i] << "\n";

    arquivo.close();

    cout << "Arquivo de ocupacao gerado: " << nomeArquivo << endl;

    delete livres;
    delete ocupados;

    return 0;
}