#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h> // funções POSIX (read, write, close)
#include <signal.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h> // funções de rede (sockaddr_in, inet_pton)
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <locale.h>

#include "../include/proto.h" // constantes e mensagens definidas para o projeto (porta, mensagens de erro)

static int listen_fd = -1; //guarda o socket principal de escuta

/*
MUST:
Servidor (server):
- aceita conexões TCP; 
-recebe mensagens de requisição, separa tokens e executa as quatro operações básicas (+, −, ×, ÷); 
- devolve o resultado ao cliente.
*/

// alguns handlers de sinais, como sigchld (um processo filho termina)
void sigchld_handler(int signo) {
    (void)signo;
    while (waitpid(-1, NULL, WNOHANG) > 0) { //waitpid com WNOHANG -> limpa os filhos "zumbis"
    }
}

void sigint_handler(int signo) { //sigint eh chamado quando o usuario pressiona CTRL + C
    (void)signo;
    if (listen_fd != -1) close(listen_fd);
    fprintf(stderr, "\n[server] encerrando por SIGINT\n");
    exit(0);
}

//algumas funcoes para tratar inputs do cliente


char *trim(char *s) { //remove espaços em branco no começo e fim de uma string
    if (!s) return s;
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

/*
parse_expr tenta interpretar a linha enviada pelo cliente.
Dois modos possíveis:
    prefixo → "ADD 10 20"
    infixo → "10 + 20"
Usa strtok_r para dividir em tokens.
Converte números com strtod.
Retorna 0 se OK, -1 se inválido.
Escreve:
    op_out → operador (+, -, *, /)
    a_out, b_out → operandos */

int parse_expr(const char *line, char *op_out, double *a_out, double *b_out) {
    if (!line) return -1;
    char buf[BUFSIZE];
    strncpy(buf, line, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';
    char *p = trim(buf);
    if (strlen(p) == 0) return -1;

    char *tokens[4];
    int n = 0;
    char *save;
    char *t = strtok_r(p, " \t\r\n", &save);
    while (t && n < 4) {
        tokens[n++] = t;
        t = strtok_r(NULL, " \t\r\n", &save);
    }

    if (n == 3) {

        if (strcasecmp(tokens[0], "ADD") == 0 || strcasecmp(tokens[0], "SUB") == 0 ||
            strcasecmp(tokens[0], "MUL") == 0 || strcasecmp(tokens[0], "DIV") == 0) {
            char *end1, *end2;
            errno = 0;
            double a = strtod(tokens[1], &end1);
            double b = strtod(tokens[2], &end2);
            if (end1 == tokens[1] || *end1 != '\0' || end2 == tokens[2] || *end2 != '\0' || errno == ERANGE)
                return -1;

            if (strcasecmp(tokens[0], "ADD") == 0) *op_out = '+';
            if (strcasecmp(tokens[0], "SUB") == 0) *op_out = '-';
            if (strcasecmp(tokens[0], "MUL") == 0) *op_out = '*';
            if (strcasecmp(tokens[0], "DIV") == 0) *op_out = '/';
            *a_out = a; *b_out = b;
            return 0;
        }
    }

    if (n == 3) {
        if (strlen(tokens[1]) == 1) {
            char c = tokens[1][0];
            if (c=='+'||c=='-'||c=='*'||c=='/') {
                char *end1, *end2;
                errno = 0;
                double a = strtod(tokens[0], &end1);
                double b = strtod(tokens[2], &end2);
                if (end1 == tokens[0] || *end1 != '\0' || end2 == tokens[2] || *end2 != '\0' || errno == ERANGE)
                    return -1;
                *op_out = c; *a_out = a; *b_out = b; return 0;
            }
        }
    }

    return -1;
}

/*
- atende um único cliente
- exibe log com IP e porta do cliente
- cria FILE *f = fdopen(fd, "r+") → transforma socket em fluxo estilo arquivo (facilita fgets, fputs).
- loop:
    Lê linha (fgets).
    Aplica trim.
    Se for "QUIT" → encerra conexão.
    Chama parse_expr.
    Se erro → responde ERR EINV.
    Se divisão por zero → responde ERR EZDV.
    Caso contrário → calcula resultado.
- monta resposta "OK resultado\n".
- envia para cliente com fputs.
- fecha conexão (fclose(f)).
*/

void handle_client(int fd, struct sockaddr_in *peer) {
    char addrbuf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer->sin_addr, addrbuf, sizeof(addrbuf));
    int port = ntohs(peer->sin_port);
    fprintf(stderr, "[server] conexao aceitada de %s:%d (pid %d)\n", addrbuf, port, getpid());

    FILE *f = fdopen(fd, "r+");
    if (!f) {
        close(fd);
        return;
    }
    char line[BUFSIZE];
    setvbuf(f, NULL, _IOLBF, 0);

    while (fgets(line, sizeof(line), f)) {
        char *ln = trim(line);
        if (strlen(ln) == 0) continue;
        fprintf(stderr, "[server] [%s:%d] requisicao: '%s'\n", addrbuf, port, ln);

        if (strcasecmp(ln, "QUIT") == 0) {
            fprintf(stderr, "[server] [%s:%d] cliente pediu QUIT\n", addrbuf, port);
            break;
        }

        char op;
        double a,b;
        if (parse_expr(ln, &op, &a, &b) != 0) {
            fputs(ERR_EINV, f);
            fprintf(stderr, "[server] [%s:%d] entrada invalida\n", addrbuf, port);
            continue;
        }

        if (op == '/' && b == 0.0) {
            fputs(ERR_EZDV, f);
            fprintf(stderr, "[server] [%s:%d] divisao por zero\n", addrbuf, port);
            continue;
        }

        double r;
        switch(op) {
            case '+': r = a + b; break;
            case '-': r = a - b; break;
            case '*': r = a * b; break;
            case '/': r = a / b; break;
            default:
                fputs(ERR_ESRV, f);
                continue;
        }

        char out[128];
        snprintf(out, sizeof(out), "OK %.6f\n", r);
        fputs(out, f);
    }

    fclose(f);
    fprintf(stderr, "[server] conexao %s:%d fechada (pid %d)\n", addrbuf, port, getpid());
}

int main(int argc, char **argv) {
    setlocale(LC_NUMERIC, "C"); 

    unsigned short port = DEFAULT_PORT; // permite definir porta pela linha de comando (./server 6000)
    if (argc >= 2) {
        long p = strtol(argv[1], NULL, 10);
        if (p > 0 && p <= 65535) port = (unsigned short)p;
    }

    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);


    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    /*
    AF_INET → IPv4
    SOCK_STREAM → TCP
    */

    if (sockfd < 0) { perror("socket"); exit(1); }
    listen_fd = sockfd;

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // -> permite reutilizar porta rapidamente se o servidor for reiniciado

    struct sockaddr_in serv;
    memset(&serv,0,sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&serv, sizeof(serv)) < 0) { perror("bind"); close(sockfd); exit(1); }
    if (listen(sockfd, BACKLOG) < 0) { perror("listen"); close(sockfd); exit(1); }

    fprintf(stderr, "[server] ouvindo na porta %d\n", port);

    for (;;) {
        struct sockaddr_in peer;
        socklen_t plen = sizeof(peer);
        int fd = accept(sockfd, (struct sockaddr*)&peer, &plen);
        if (fd < 0) {
            if (errno == EINTR) continue;
            perror("accept"); break;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork"); close(fd); continue;
        }
        if (pid == 0) {
            close(sockfd);
            handle_client(fd, &peer);
            exit(0);
        } else {
            close(fd);
        }
    }

    /*
    cada cliente é atendido por um processo filho.
    pai continua aceitando novas conexões.
    filhos tratam o cliente até ele mandar QUIT ou desconectar.
    */

    close(sockfd);
    return 0;
}
