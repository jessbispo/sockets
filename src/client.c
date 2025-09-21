
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <locale.h>
#include <ctype.h>

#include "../include/proto.h"
/*
MUST:
cliente (client): 
- conecta ao servidor 
- envia a operação digitada pelo usuário (linha por linha) 
- imprime a resposta.

*/
int main(int argc, char **argv) {
    setlocale(LC_NUMERIC, "C"); // -> garante que números usem ponto . como separador decimal (e não vírgula, que poderia confundir o servidor)

    if (argc < 3) {
        fprintf(stderr, "Uso: %s <host> <port>\n", argv[0]);
        return 1;
    } // garante que recebeu os argumentos necessarios
    const char *host = argv[1];
    const char *port = argv[2];

    struct addrinfo hints, *res, *rp;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo"); return 1;
    }

    int sfd = -1;
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) continue;
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(sfd); sfd = -1;
    }
    freeaddrinfo(res);
    if (sfd == -1) { fprintf(stderr, "nao foi possivel conectar\n"); return 1; } 
    /*
    tenta criar um socket e conectar a cada endereço retornado pelo getaddrinfo.
        se conseguir conectar → sai do loop.
        se nenhum endereço funcionar → erro.
    */

    FILE *sockf = fdopen(sfd, "r+"); // fdopen transforma o socket em um FILE*, assim dá pra usar fgets, fputs (mais simples que read/write)
    if (!sockf) { perror("fdopen"); close(sfd); return 1; }
    setvbuf(sockf, NULL, _IOLBF, 0); // envia a cada \n

    char buf[BUFSIZE];
    while (fgets(buf, sizeof(buf), stdin)) {

        // envia pro servidor
        size_t L = strlen(buf);
        if (L == 0) continue;
        if (buf[L-1] != '\n') {

            buf[L] = '\n'; buf[L+1] = '\0';
        }
        if (fputs(buf, sockf) == EOF) { perror("write"); break; }
        fflush(sockf);

        //espera uma resposta
        char resp[BUFSIZE];
        if (!fgets(resp, sizeof(resp), sockf)) {
            fprintf(stderr, "conexao fechada pelo servidor\n"); break;
        }
        printf("%s", resp);
        char tmp[BUFSIZE];
        strncpy(tmp, buf, sizeof(tmp)-1); tmp[sizeof(tmp)-1]='\0';
        char *t = tmp;
        while (*t && *t!='\n') t++; *t='\0';

        while (*t && *t!='\n') {
            t++;
        }

        *t = '\0';
    }

    fclose(sockf);
    return 0;
}
