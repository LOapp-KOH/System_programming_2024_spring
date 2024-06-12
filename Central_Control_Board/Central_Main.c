
#include "JaeYukBookkom.h"
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
#include <pthread.h>
#include "LCD.c"
#include "GPIO.c"
#include "Networking.c"
#include "PWM.c"

short int Window_Open = 0;
short int problem_sensing = 0;
short int user = 0;
void *Communicating(void *args);
int ms_handle_sense(void);
int ms_handle_oper(void);
int toggle_window(void);
void *getFromAPI(void *args);
void *AutoRefrash(void *args);
struct timeval timeout = {0, 1000};
pthread_t *mainThread, *APIthread, *TimerThread;
int main(int argc, char *argv[]){
    if (argc != 2){
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    ServerInit(&serv_sock, atoi(argv[1]));
    GPIOInit();
    LCD_init();
    mainThread = malloc(sizeof(pthread_t));
    APIthread = malloc(sizeof(pthread_t));
    TimerThread = malloc(sizeof(pthread_t));
    if (pthread_create(mainThread, NULL, Communicating, NULL) < 0)
        perror("main thread create error:");
    if (pthread_create(APIthread, NULL, getFromAPI, NULL) < 0)
        perror("API thread create error:");
    if (pthread_create(TimerThread, NULL, AutoRefrash, NULL) < 0)
        perror("API thread create error:");

    pthread_join(*mainThread, NULL);
    pthread_join(*APIthread, NULL);
    pthread_join(*TimerThread, NULL);
    free(mainThread);
    free(APIthread);
    free(TimerThread);
    return (0);
}
void *Communicating(void *args){
    int state = 1;
    int prev_state = 1;
    while (1){
        state = GPIORead(BUTTON_IN);
        if (prev_state == 0 && state == 1) // button polling
            toggle_window();
        prev_state = state;

        clients = clients_socket;
        if (select(max_fd + 1, &clients, NULL, NULL, &timeout)){ // polling with listening clients message
            if (FD_ISSET(clnt_sock[0], &clients)) // message from sensing board
                ms_handle_sense();
            if (FD_ISSET(clnt_sock[1], &clients)) // message from sensing board
                ms_handle_oper();
        }
        usleep(500 * 100);
    }

    //dispose socket
    for (int i = 0; i < MAX_CONNECT_BOARD; i++){
        close(clnt_sock[i]);
    }
    close(serv_sock);
}
int ms_handle_sense(void){ // message handler from sensing board
    char input_str[3] = {0};
    int str_len = read(clnt_sock[0], input_str, sizeof(input_str));
    if (str_len == -1){
        error_handling("read() error");
        return -1;
    }
    problem_sensing = atoi(input_str);
    printf("problem: %d\n");
    LCD_messaging_error(problem_sensing);
    if (problem_sensing && !user && problem_sensing != 8)
        if (Window_Open)
            toggle_window();
}
int ms_handle_oper(void){ // message handler from operation board
    char input_str[3] = {0};
    int str_len = read(clnt_sock[1], input_str, sizeof(input_str));
    if (str_len == -1){
        error_handling("read() error");
        return -1;
    }
    int danger = atoi(input_str);
    if (danger){
        problem_sensing = 8;
        LCD_messaging_error(problem_sensing);
        boozerThread = malloc(sizeof(pthread_t));
        if (pthread_create(boozerThread, NULL, bozzering, NULL) < 0)
            perror("buzzor thread create error:");
    }
}
int toggle_window(void){
    printf("Window toggle\n");
    if (problem_sensing == 8){
        pthread_cancel(*boozerThread); // boozer off
        problem_sensing = 0;
        LCD_messaging_error(problem_sensing);
        return -1;
    }
    else if (problem_sensing && !Window_Open){
        boozer_Warning();
        user = 1;
    }
    Window_Open = !Window_Open;
    char output_str[3] = {0};
    snprintf(output_str, 3, "%d", Window_Open);
    write(clnt_sock[1], output_str, sizeof(output_str)); // open /close call to operation board

    printf("msg = %s\n", output_str);
    if (Window_Open)
        GPIOWrite(BUTTON_LED, HIGH);
    else{
        GPIOWrite(BUTTON_LED, LOW);
        user = 0;
    }
        
}
void *getFromAPI(void *args){
    fd_set API_socket, API;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;
    FD_ZERO(&API_socket);
    FD_SET(clnt_sock[2], &API_socket);
    int max_fd2 = clnt_sock[2];
    int str_len;
    char str[50];
    float temp, hyd;
    int weatherCode, dust, rainPer;
    while (1){
        if (select(max_fd2 + 1, &clients, NULL, NULL, &timeout)){ // polling with listening clients message
            if (FD_ISSET(clnt_sock[2], &clients)){ // message from sensing board
                str_len = read(clnt_sock[2], str, sizeof(str));
                if (str_len == -1){
                    error_handling("read() error");
                    continue;
                }
                printf("Receive message from Client : %s\n", str);

                char *ptr = strtok(str, " ");
                printf("temp: %s\n", ptr);
                temp = atof(ptr);
                ptr = strtok(NULL, " ");
                printf("hye: %s\n", ptr);
                hyd = atof(ptr);
                ptr = strtok(NULL, " ");
                printf("wheatherCode: %s\n", ptr);
                weatherCode = atoi(ptr);
                ptr = strtok(NULL, " ");
                printf("dust: %s\n", ptr);
                dust = atoi(ptr);
                ptr = strtok(NULL, " ");
                printf("rainPer: %s\n", ptr);
                rainPer = atoi(ptr);
                LCD_weather_prediction(temp, hyd, weatherCode, dust, rainPer);
                printf("API data received\n");
            }
        }
        usleep(5000000);
    }
}
void *AutoRefrash(void *args){
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    int state = 1, prev_state = 1;
    printf("refresh sensing start");
    while (1){
        state = GPIORead(TIMER_IN);
        if (prev_state == 0 && state == 1){ // button polling
            printf("Auto reFrash with 15s\n");
            if (!Window_Open && !problem_sensing)// when window closed >> op -> cl
                toggle_window();
            usleep(15000000); // waiting 15min(alternative: sec) with window open
            if (problem_sensing != 8 && Window_Open)// if not door interrupt and not user close excuted
                toggle_window();
        }
        prev_state = state;
        usleep(50000);
    }
}