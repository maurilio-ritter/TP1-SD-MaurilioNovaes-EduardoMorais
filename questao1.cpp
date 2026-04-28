#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <random>
#include <cstring>

constexpr int MSG_SIZE = 20;

bool isPrime(long long n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (long long i = 3; i * i <= n; i += 2)
        if (n % i == 0) return false;
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <quantidade>\n";
        return 1;
    }

    int qtd = std::stoi(argv[1]);
    int fd[2];

    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // Consumidor
        close(fd[1]);

        char buffer[MSG_SIZE + 1];

        while (true) {
            ssize_t bytes = read(fd[0], buffer, MSG_SIZE);
            if (bytes <= 0) break;

            buffer[MSG_SIZE] = '\0';
            long long num = std::stoll(buffer);

            if (num == 0) break;

            std::cout << num << " -> "
                      << (isPrime(num) ? "primo" : "nao primo")
                      << std::endl;
        }

        close(fd[0]);
        return 0;
    } else {
        // Produtor
        close(fd[0]);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(1, 100);

        long long n = 1;

        for (int i = 0; i < qtd; i++) {
            char msg[MSG_SIZE + 1];
            snprintf(msg, sizeof(msg), "%020lld", n);
            write(fd[1], msg, MSG_SIZE);

            n += dist(gen);
        }

        char fim[MSG_SIZE + 1];
        snprintf(fim, sizeof(fim), "%020d", 0);
        write(fd[1], fim, MSG_SIZE);

        close(fd[1]);
        wait(nullptr);
    }

    return 0;
}