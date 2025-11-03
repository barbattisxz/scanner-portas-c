#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int set_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <IP> <porta_inicio> <porta_fim> [timeout_ms]\n", argv[0]);
        fprintf(stderr, "Exemplo: %s 127.0.0.1 100 200 200\n", argv[0]);
        return 1;
    }

    const char *ip_alvo = argv[1];
    int porta_inicio = atoi(argv[2]);
    int porta_fim = atoi(argv[3]);
    int timeout_ms = 200; // padrão
    if (argc >= 5) timeout_ms = atoi(argv[4]);

    if (porta_inicio <= 0 || porta_fim <= 0 || porta_inicio > porta_fim) {
        fprintf(stderr, "Intervalo de portas inválido.\n");
        return 1;
    }
    if (timeout_ms <= 0) timeout_ms = 200;

    FILE *arquivo = fopen("resultado.txt", "w");
    if (arquivo == NULL) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    // escreve um cabeçalho com timestamp
    time_t now = time(NULL);
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(arquivo, "Scan em %s - alvo: %s - portas %d..%d - timeout %d ms\n\n",
            timestr, ip_alvo, porta_inicio, porta_fim, timeout_ms);
    fflush(arquivo);

    struct sockaddr_in destino;
    memset(&destino, 0, sizeof(destino));
    destino.sin_family = AF_INET;

    if (inet_pton(AF_INET, ip_alvo, &destino.sin_addr) <= 0) {
        fprintf(stderr, "IP inválido: %s\n", ip_alvo);
        fclose(arquivo);
        return 1;
    }

    printf("Iniciando scanner %s portas %d..%d (timeout %d ms)\n",
           ip_alvo, porta_inicio, porta_fim, timeout_ms);

    for (int porta = porta_inicio; porta <= porta_fim; porta++) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("socket");
            fclose(arquivo);
            return 1;
        }

        destino.sin_port = htons(porta);

        if (set_nonblocking(sock) < 0) {
            perror("fcntl nonblocking");
            close(sock);
            fclose(arquivo);
            return 1;
        }

        int conn = connect(sock, (struct sockaddr*)&destino, sizeof(destino));
        int status_known = 0;
        const char *status_str = "FECHADA";

        if (conn == 0) {
            // conexão imediata
            status_known = 1;
            status_str = "ABERTA";
        } else if (conn < 0 && errno != EINPROGRESS) {
            // erro imediato -> porta fechada/rejeitada
            status_known = 1;
            status_str = "FECHADA";
        }

        if (!status_known) {
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(sock, &wfds);

            struct timeval tv;
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;

            int sel = select(sock + 1, NULL, &wfds, NULL, &tv);
            if (sel > 0 && FD_ISSET(sock, &wfds)) {
                int so_error = 0;
                socklen_t len = sizeof(so_error);
                if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
                    status_str = "FECHADA";
                } else {
                    if (so_error == 0) {
                        status_str = "ABERTA";
                    } else {
                        status_str = "FECHADA";
                    }
                }
            } else if (sel == 0) {
                // timeout
                status_str = "TIMEOUT";
            } else {
                // select erro
                status_str = "FECHADA";
            }
        }

        // volta blocking por segurança e fecha
        set_blocking(sock);
        close(sock);

        // imprimir e gravar
        printf("Porta %d : %s\n", porta, status_str);
        fprintf(arquivo, "Porta %d : %s\n", porta, status_str);
        fflush(arquivo);
    }

    printf("\nScanner finalizado. Resultados salvos em resultado.txt\n");
    fclose(arquivo);
    return 0;
}
