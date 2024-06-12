#include "JaeYukBookkom.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

void error_handling(char *message);
static int GPIOExport(int pin);
static int GPIOUnexport(int pin);
static int GPIODirection(int pin, int dir);
static int GPIORead(int pin);
static int GPIOWrite(int pin, int value);

int GPIOInit(void){
    if (-1 == GPIOExport(BUTTON_IN) || -1 == GPIOExport(BUTTON_OUT)|| -1 == GPIOExport(BUTTON_LED)||-1 == GPIOExport(TIMER_IN) || -1 == GPIOExport(TIMER_OUT))
        return (1);
    if (-1 == GPIODirection(BUTTON_IN, IN) || -1 == GPIODirection(BUTTON_OUT, OUT) || -1 == GPIODirection(BUTTON_LED, OUT)||-1 == GPIODirection(TIMER_IN, IN) || -1 == GPIODirection(TIMER_OUT, OUT))
        return (2);
    if (-1 == GPIOWrite(BUTTON_OUT, 1)||-1 == GPIOWrite(TIMER_OUT, 1))
        return (3);
}
void GPIODispose(void){
    GPIOUnexport(BUTTON_IN);
    GPIOUnexport(BUTTON_OUT);
    GPIOUnexport(BUTTON_LED);
    GPIOUnexport(TIMER_IN);
    GPIOUnexport(TIMER_OUT);
}

static int GPIOExport(int pin){
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open export for writing!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return (0);
}
static int GPIOUnexport(int pin){
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open unexport for writing!\n");
        return (-1);
    }
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return (0);
}
static int GPIODirection(int pin, int dir){
    static const char s_directions_str[] = "in\0out";

    char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
    int fd;

    snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open gpio direction for writing!\n");
        return (-1);
    }

    if (-1 ==
        write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3))
    {
        fprintf(stderr, "Failed to set direction!\n");
        return (-1);
    }

    close(fd);
    return (0);
}
static int GPIORead(int pin){
    char path[VALUE_MAX];
    char value_str[3];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_RDONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open gpio value for reading!\n");
        return (-1);
    }

    if (-1 == read(fd, value_str, 3))
    {
        fprintf(stderr, "Failed to read value!\n");
        return (-1);
    }

    close(fd);

    return (atoi(value_str));
}
static int GPIOWrite(int pin, int value){
    static const char s_values_str[] = "01";
    char path[VALUE_MAX];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open gpio value for writing!\n");
        close(fd);
        return (-1);
    }

    if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1))
    {
        fprintf(stderr, "Failed to write value!\n");
        close(fd);
        return (-1);
    }
    close(fd);
    return (0);
}
