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
int inPos, outPos, ocupacao;

counting_semaphore<>* livres;
counting_semaphore<>* ocupados;

mutex mtx;
atomic<int> produzidos;
atomic<int> consumidos;

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
        if (id >= M) break;

        int numero = dist(gen);

        livres->acquire();

        {
            lock_guard<mutex> lock(mtx);
            bufferCompartilhado[inPos] = numero;
            inPos = (inPos + 1) % N;
            ocupacao++;
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
        }

        livres->release();

        isPrime(numero); // processamento

        int atual = consumidos.fetch_add(1) + 1;

        if (atual >= M) {
            for (int i = 0; i < Nc; i++)
                ocupados->release();
            break;
        }
    }
}

// Executa um cenário e retorna tempo
double executar() {
    bufferCompartilhado.assign(N, 0);
    inPos = outPos = ocupacao = 0;

    produzidos = 0;
    consumidos = 0;

    livres = new counting_semaphore<>(N);
    ocupados = new counting_semaphore<>(0);

    vector<thread> produtores;
    vector<thread> consumidores;

    auto inicio = chrono::high_resolution_clock::now();

    for (int i = 0; i < Np; i++)
        produtores.emplace_back(produtor);

    for (int i = 0; i < Nc; i++)
        consumidores.emplace_back(consumidor);

    for (auto& t : produtores) t.join();
    for (auto& t : consumidores) t.join();

    auto fim = chrono::high_resolution_clock::now();

    delete livres;
    delete ocupados;

    chrono::duration<double> tempo = fim - inicio;
    return tempo.count();
}

int main() {
    vector<int> Ns = {1, 10, 100, 1000};
    vector<pair<int,int>> configs = {
        {1,1},{1,2},{1,4},{1,8},
        {2,1},{4,1},{8,1}
    };

    ofstream arquivo("resultados.csv");
    arquivo << "N,Np,Nc,tempo_medio\n";

    for (int n : Ns) {
        N = n;

        for (auto [p, c] : configs) {
            Np = p;
            Nc = c;

            double soma = 0.0;

            for (int i = 0; i < 10; i++) {
                double t = executar();
                soma += t;
            }

            double media = soma / 10.0;

            cout << "N=" << N
                 << " Np=" << Np
                 << " Nc=" << Nc
                 << " -> " << media << " s\n";

            arquivo << N << ","
                    << Np << ","
                    << Nc << ","
                    << media << "\n";
        }
    }

    arquivo.close();

    cout << "\nArquivo 'resultados.csv' gerado!\n";

    return 0;
}
