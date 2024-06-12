#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>

#define BUFFER_MAX 3
#define DIRECTION_MAX 256
#define VALUE_MAX 256

#define IN 0
#define OUT 1

#define LOW 0
#define HIGH 1
void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int main(int argc, char *argv[]){
    int sock;
    struct sockaddr_in serv_addr;
    int str_len;
    if (argc != 2){
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    printf("Connection established\n");

    char str[50];
    while (1)
    {
        float temperature, hyderation;
        int dust, rainPer;
        int weatherCode;
        printf("input temperature, hyderation: ");
        scanf("%f %f",&temperature,&hyderation);
        printf("input Whether prediction(1.Sunny,2.Cloud,3.Windy,4.Rain,5.Snow)\n:");
        scanf("%d",&weatherCode);
        printf("input dust, rainPer:");
        scanf("%d %d",&dust,&rainPer);

        snprintf(str, 50,"%.2f %.2f %d %d %d", temperature, hyderation, weatherCode, dust, rainPer);
        int bytes_written = write(sock, str, sizeof(str));
        if (bytes_written == -1)
            error_handling("write() error");
        usleep(500 * 100);
        printf("Write success: %s\n",str);
    }

    close(sock);

    return (0);
}