#include "JaeYukBookkom.h"
#include <pthread.h>
#include <fcntl.h>
#include <stdio.h>

static int PWMExport(int pwmnum);
static int PWMUnExport(int pwmnum);
static int PWMEnable(int pwmnum);
static int PWMWritePeriod(int pwmnum, int value);
static int PWMWriteDutyCycle(int pwmnum, int value);
int PWMInit(int pwm, int period, int dutycycle);
void *bozzering(void *arg);
void boozer_Warning(void);
void SpeakerUnExport(void *args);

int boozer_on = 0;
pthread_t *boozerThread;
int PWMInit(int pwm, int period, int dutycycle){
    PWMExport(pwm);
    PWMWritePeriod(pwm, period);
    PWMWriteDutyCycle(pwm, dutycycle);
}
void *bozzering(void *arg){
    PWMInit(SPEAKER,10000000,0);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_cleanup_push(SpeakerUnExport, NULL);
    while (1){
        for (int i = 0; i < 1000; i++){
            PWMWriteDutyCycle(SPEAKER, i * 10000);
            usleep(1000);
        }
        for (int i = 1000; i > 0; i--){
            PWMWriteDutyCycle(SPEAKER, i * 10000);
            usleep(1000);
        }
    }
    pthread_cleanup_pop(0);
}
void boozer_Warning(void){
    PWMInit(SPEAKER,10000000,0);
    for (int i = 0; i < 1000; i++){
        PWMWriteDutyCycle(SPEAKER, i * 10000);
        usleep(1000);
    }
    for (int i = 1000; i > 0; i--){
        PWMWriteDutyCycle(SPEAKER, i * 10000);
        usleep(1000);
    }
    PWMUnExport(SPEAKER);
}
static int PWMExport(int pwmnum){
    char buffer[BUFFER_MAX];
    int fd, byte;

    // TODO: Enter the export path.
    fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
    if (-1 == fd){
        fprintf(stderr, "Failed to open export for export!\n");
        return (-1);
    }

    byte = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
    write(fd, buffer, byte);
    close(fd);
    
    usleep(1000000);
    return (0);
}
static int PWMUnExport(int pwmnum){
    char buffer[BUFFER_MAX];
    int fd, byte;
    // TODO: Enter the export path.
    fd = open("/sys/class/pwm/pwmchip0/unexport", O_WRONLY);
    if (-1 == fd){
        fprintf(stderr, "Failed to open export for export!\n");
        return (-1);
    }
    byte = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
    write(fd, buffer, byte);
    close(fd);
    
    usleep(1000);
    return (0);
}
void SpeakerUnExport(void *args){
    PWMUnExport(SPEAKER);
    free(boozerThread);
    sleep(1);
}
static int PWMEnable(int pwmnum){
    static const char s_enable_str[] = "1";

    char path[DIRECTION_MAX];
    int fd;

    // TODO: Enter the enable path.
    snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm0/enable", pwmnum);
    fd = open(path, O_WRONLY);
    if (-1 == fd){
        fprintf(stderr, "Failed to open in enable!\n");
        return -1;
    }

    write(fd, s_enable_str, strlen(s_enable_str));
    close(fd);

    return (0);
}
static int PWMWritePeriod(int pwmnum, int value){
    char s_value_str[VALUE_MAX];
    char path[VALUE_MAX];
    int fd, byte;

    // TODO: Enter the period path.
    snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm0/period", pwmnum);
    fd = open(path, O_WRONLY);
    if (-1 == fd){
        fprintf(stderr, "Failed to open in period!\n");
        return (-1);
    }
    byte = snprintf(s_value_str, VALUE_MAX, "%d", value);

    if (-1 == write(fd, s_value_str, byte)){
        fprintf(stderr, "Failed to write value in period!\n");
        close(fd);
        return -1;
    }
    close(fd);

    return (0);
}
static int PWMWriteDutyCycle(int pwmnum, int value)
{
    char s_value_str[VALUE_MAX];
    char path[VALUE_MAX];
    int fd, byte;

    // TODO: Enter the duty_cycle path.
    snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm0/duty_cycle", pwmnum);
    fd = open(path, O_WRONLY);
    if (-1 == fd){
        fprintf(stderr, "Failed to open in duty cycle!\n");
        return (-1);
    }
    byte = snprintf(s_value_str, VALUE_MAX, "%d", value);

    if (-1 == write(fd, s_value_str, byte)){
        fprintf(stderr, "Failed to write value in duty cycle!\n");
        close(fd);
        return -1;
    }
    close(fd);

    return (0);
}
