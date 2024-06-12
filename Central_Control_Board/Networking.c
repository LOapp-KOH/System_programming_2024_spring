#include "JaeYukBookkom.h"
#include <arpa/inet.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
void ServerInit(int *serv_sock, int port);
int max(int *list, int length);
void error_handling(char *message);

int clnt_sock[MAX_CONNECT_BOARD];
int serv_sock = -1, max_fd;
fd_set clients_socket, clients;

void ServerInit(int *serv_sock, int port){
    for (int i = 0; i < MAX_CONNECT_BOARD; i++){
        clnt_sock[i] = -1;
    }
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr[MAX_CONNECT_BOARD];
    socklen_t clnt_addr_size;
    *serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (*serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (bind(*serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(*serv_sock, 5) == -1)
        error_handling("listen() error");
    for (int i = 0; i < MAX_CONNECT_BOARD; i++){
        if (clnt_sock[i] < 0){
            clnt_addr_size = sizeof(clnt_addr[i]);
            clnt_sock[i] =
                accept(*serv_sock, (struct sockaddr *)&clnt_addr[i], &clnt_addr_size);
            if (clnt_sock[i] == -1)
                error_handling("accept() error");
        }
        printf("Connection established with client %d\n",i);
    }
    printf("Connection End\n");

    FD_ZERO(&clients_socket);
    for (int i = 0; i < MAX_CONNECT_BOARD; i++){
        FD_SET(clnt_sock[i], &clients_socket);
    }
    
    max_fd = max(clnt_sock, MAX_CONNECT_BOARD);
}
int max(int *list, int length){
    int temp = list[0];
    for (int i = 1; i < length; i++){
        if (list[i - 1] < list[i])
            temp = list[i];
    }
    return temp;
}
void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
