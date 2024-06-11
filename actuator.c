
#include <pthread.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define IN 0
#define OUT 1
#define PWM 0
#define SPOUTO 23
#define SPINO 24
#define SPOUTC 27
#define SPINC 22
#define REDPIN 17

#define BUFFER_MAX 3
#define LOW 0
#define HIGH 1
#define VALUE_MAX 256
#define DIRECTION_MAX 256

#define SLEEP_DURATION 50000 // 10ms 대기 시간
#define PWM_PIN 0
#define PERIOD 20000000 // 20ms in nanoseconds

void error_handling(char *message) {
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}

static int GPIOExport(int pin);
static int GPIOUnexport(int pin);
static int GPIODirection(int pin, int dir);
static int GPIORead(int pin);
static int GPIOWrite(int pin, int value);
static int PWMExport(int pwmnum);
static int PWMEnable(int pwmnum);
static int PWMWritePeriod(int pwmnum, int value);
static int PWMWriteDutyCycle(int pwmnum, int value);

char ms[2];
int cmd=0;//명령
int red=0;//적외선 센서 사람감지
int super=0;//초음파 센서 물체 감지
int motor=0;//모터 작동  1이면 작동중
int targc=0;
int sock;

void *supersonicopen(){
  clock_t start_t, end_t;
  double time;
  if (-1 == GPIOExport(SPOUTO) || -1 == GPIOExport(SPINO)) {
    printf("gpio export err\n");
  }
  if (-1 == GPIODirection(SPOUTO, OUT) || -1 == GPIODirection(SPINO, IN)) {
    printf("gpio direction err\n");
  }
  GPIOWrite(SPOUTO, 0);
  usleep(10000);

  double distance=0;

  while(1){
    if(motor==1){
      break;
    }
  }
  while(1){
    if(motor==0){
      break;
    }

    if (-1 == GPIOWrite(SPOUTO, 1)) {
      printf("gpio write/trigger err\n");
    }
    usleep(10);
    GPIOWrite(SPOUTO, 0);
    while (GPIORead(SPINO) == 0) {
      start_t = clock();
    }
    while (GPIORead(SPINO) == 1) {
      end_t = clock();
    }

    time = (double)(end_t - start_t) / CLOCKS_PER_SEC;  // ms
    //printf("time : %.4lf\n", time);
    //printf("distance : %.2lfcm\n", time / 2 * 34000);
    distance=time / 2 * 34000;
    if(distance<4){//물체 감지 범위 정해서
      super=1;
    }
    usleep(500000);
  }
  pthread_exit(NULL);
}

void *supersonicclose(){
  clock_t start_t, end_t;
  double time;
  if (-1 == GPIOExport(SPOUTC) || -1 == GPIOExport(SPINC)) {
    printf("gpio export err\n");
  }
  if (-1 == GPIODirection(SPOUTC, OUT) || -1 == GPIODirection(SPINC, IN)) {
    printf("gpio direction err\n");
  }
  GPIOWrite(SPOUTC, 0);
  usleep(10000);

  double distance=0;

  while(1){
    if(motor==1){
      break;
    }
  }
  while(1){
    if(motor==0){
      break;
    }

    if (-1 == GPIOWrite(SPOUTC, 1)) {
      printf("gpio write/trigger err\n");
    }
    usleep(10);
    GPIOWrite(SPOUTC, 0);
    while (GPIORead(SPINC) == 0) {
      start_t = clock();
    }
    while (GPIORead(SPINC) == 1) {
      end_t = clock();
    }

    time = (double)(end_t - start_t) / CLOCKS_PER_SEC;  // ms
    //printf("time : %.4lf\n", time);
    //printf("distance : %.2lfcm\n", time / 2 * 34000);
    distance=time / 2 * 34000;
    if(distance<4){//물체 감지 범위 정해서
      super=1;
    }
    usleep(500000);
  }
  pthread_exit(NULL);
}

void *redsensor(){
  if (-1 == GPIOExport(REDPIN)) {
    printf("gpio export err\n");
  }
  usleep(10000);
  if (-1 == GPIODirection(REDPIN, IN)) {
    printf("gpio direction err\n");
  }
  usleep(10000);
  while(1){
    if(motor==1){
      break;
    }
  }
  while(1){
    if(motor==0){
      break;
    }
    //printf("GPIORead: %d from pin %d\n", GPIORead(REDPIN), REDPIN);
    if(GPIORead(REDPIN)==1){
      red=1;
      
    }
    usleep(500000);
  }

  pthread_exit(NULL);
}
/*
void *test(){
  while(1){scanf("%d",&cmd);}
}*/

void *com(void*arg){
    char** argv = (char**)arg;
    int state = 1;
    int prev_state = 1;
    int light = 0;

    struct sockaddr_in serv_addr;
    char msg[2];
    char on[2] = "1";
    int str_len;

    if (targc != 3)
    {
      printf("Usage : %s <IP> <port>\n", argv[0]);
      exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    printf("Connection established\n");
    fd_set server_socket, server;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;
    FD_ZERO(&server_socket);
    FD_SET(sock, &server_socket);
    int max_fd = sock;
    int result = 0;
    char str[20];
    while (1)
    {
        server = server_socket;
        result = select(max_fd + 1, &server, NULL, NULL, &timeout);//읽을 데이터 있는지 확인
        if (result)
        {
            str_len = read(sock, str, sizeof(str));
            if (str_len == -1){
              error_handling("read() error");
            }
            else if(atoi(str)==1){
              cmd=1;
            }else if(atoi(str)==0){
              cmd=0;
            }
            printf("Receive message from Server : %s\n", str);
        }
        
        usleep(500 * 100);
    }
}

int main(int argc, char *argv[]){//메인메인메인메인메인메인메인메인메인메인메인메인메인메인메인메인메인메인메인메인메인메인메인
  pthread_t p_thread[4];
  int thr_id;
  char p0[] = "supersonicclose";
  char p1[] = "supersonicopen";
  char p2[] = "redsensor";
  char p3[] = "com";
  pthread_t pt;
  pthread_t pcom;
  //thr_id = pthread_create(&pt, NULL, test, (void *)p1);
  if (thr_id < 0) {
    perror("thread create error : ");
    exit(0);
  }
  targc=argc;
  thr_id = pthread_create(&pcom, NULL, com, (void *)argv);
  if (thr_id < 0) {
    perror("thread create error : ");
    exit(0);
  }
  
  PWMExport(PWM);
  PWMWriteDutyCycle(PWM_PIN, 0);
  PWMEnable(PWM);

  for (int angle = 0; angle >= -180; angle -= 10) {
      int duty_cycle = 1000000 + (angle * 1000000 / 180); // 1ms to 2ms
      if (PWMWriteDutyCycle(PWM_PIN, duty_cycle) == -1) {
          return -1;
      }
      usleep(SLEEP_DURATION); // Wait for 10ms
    }
  
  int tcmd=0;
  int num=0;

  while(1){
    int speed=10;
    char var[3];
    num=0;

    if(cmd!=tcmd&&cmd==1){
      thr_id = pthread_create(&p_thread[1], NULL, supersonicopen, (void *)p1);
      if (thr_id < 0) {
        perror("thread create error : ");
        exit(0);
      }
      thr_id = pthread_create(&p_thread[2], NULL, redsensor,(void*)p2);
      if (thr_id < 0) {
        perror("thread create error : ");
        exit(0);
      }
      usleep(20000);//센서들 세팅 대기
      //모터 작동
      motor=1;
      int a=0;
      for (int angle = -180; angle <= 310; angle += speed) {
        int duty_cycle = 1000000 + (angle * 1000000 / 180); // 1ms to 2ms
        if (PWMWriteDutyCycle(PWM_PIN, duty_cycle) == -1) {
            printf("error\n");
        }
        if(red==1&&a==0){
          printf("사람이 근처에 있습니다. 천천히 동작합니다.\n");
          speed=5;
          a++;
        }
        if(super==1){
          printf("물체 감지, 정지합니다\n");
          snprintf(var,3,"01");
          write(sock,var,sizeof(var));
          motor=0;
          break;
        }
        
        usleep(SLEEP_DURATION); // Wait for 10ms
      }
      
      motor=0;//모터 작동정지 -> 다른 센서들 작동 정지 밑에 쓰레드 조인 써야함
      
      usleep(20000);
      tcmd=1;
      speed=10;
      super=0;
      red=0;
      //printf("cmd %d tcmd %d\n",cmd, tcmd);
      pthread_join(p_thread[1],NULL);
      pthread_join(p_thread[2],NULL);

    }else if(cmd!=tcmd&&cmd==0){
      thr_id = pthread_create(&p_thread[0], NULL, supersonicclose, (void *)p0);
      if (thr_id < 0) {
        perror("thread create error : ");
        exit(0);
      }
      thr_id = pthread_create(&p_thread[2], NULL, redsensor,(void*)p2);
      if (thr_id < 0) {
        perror("thread create error : ");
        exit(0);
      }
      usleep(20000);//센서들 세팅 대기
      //모터 작동
      motor=1;
      int a=0;
      for (int angle = 310; angle >= -180; angle -= speed) {
        int duty_cycle = 1000000 + (angle * 1000000 / 180); // 1ms to 2ms
        if (PWMWriteDutyCycle(PWM_PIN, duty_cycle) == -1) {
            printf("error\n");
        }
        if(red==1&&a==0){
          printf("사람이 근처에 있습니다. 천천히 동작합니다.\n");
          speed=5;
          a++;
        }
        if(super==1){
          printf("물체 감지, 정지합니다.\n");
          snprintf(var,3,"01");
          write(sock,var,sizeof(var));
          motor=0;
          break;
        }
        
        usleep(SLEEP_DURATION); // Wait for 10ms
      }
      
      motor=0;//모터 작동정지 -> 다른 센서들 작동 정지 밑에 쓰레드 조인 써야함
      usleep(20000);
      tcmd=0;
      speed=10;
      super=0;
      red=0;
      //printf("cmd %d tcmd %d\n",cmd, tcmd);
      pthread_join(p_thread[1],NULL);
      pthread_join(p_thread[2],NULL);
    }
  }
}

static int GPIOExport(int pin) {
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/export", O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open export for writing!\n");
    return (-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return (0);
}

static int GPIOUnexport(int pin) {
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/unexport", O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open unexport for writing!\n");
    return (-1);
  }
  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return (0);
}
static int GPIODirection(int pin, int dir) {
  static const char s_directions_str[] = "in\0out";
  char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
  int fd;

  snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open gpio direction for writing!\n");
    return (-1);
  }

  if (-1 ==
      write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
    fprintf(stderr, "Failed to set direction!\n");
    return (-1);
  }

  close(fd);
  return (0);
}

static int GPIORead(int pin) {
  char path[VALUE_MAX];
  char value_str[3];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_RDONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open gpio value for reading!\n");
    return (-1);
  }

  if (-1 == read(fd, value_str, 3)) {
    fprintf(stderr, "Failed to read value!\n");
    return (-1);
  }

  close(fd);

  return (atoi(value_str));
}

static int GPIOWrite(int pin, int value) {
  static const char s_values_str[] = "01";
  char path[VALUE_MAX];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_WRONLY);

  if (-1 == fd) {
    fprintf(stderr, "Failed to open gpio value for writing!\n");
    return (-1);
  }

  if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
    fprintf(stderr, "Failed to write value!\n");
    return (-1);

    close(fd);
    return (0);
  }
}

static int PWMExport(int pwmnum) {

  char buffer[BUFFER_MAX];
  int fd, byte;

  // TODO: Enter the export path.
  fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open export for export!\n");
    return (-1);
  }

  byte = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
  write(fd, buffer, byte);
  close(fd);

  sleep(1);

  return (0);
}

static int PWMEnable(int pwmnum) {
  static const char s_enable_str[] = "1";

  char path[DIRECTION_MAX];
  int fd;

  // TODO: Enter the enable path.
  snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm0/enable", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open in enable!\n");
    return -1;
  }

  write(fd, s_enable_str, strlen(s_enable_str));
  close(fd);

  return (0);
}
static int PWMWritePeriod(int pwmnum, int value) {
  char s_value_str[VALUE_MAX];
  char path[VALUE_MAX];
  int fd, byte;

  // TODO: Enter the period path.
  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm0/period", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open in period!\n");
    return (-1);
  }
  byte = snprintf(s_value_str, VALUE_MAX, "%d", value);

  if (-1 == write(fd, s_value_str, byte)) {
    fprintf(stderr, "Failed to write value in period!\n");
    close(fd);
    return -1;
  }
  close(fd);

  return (0);
}

static int PWMWriteDutyCycle(int pwmnum, int value) {
  char s_value_str[VALUE_MAX];
  char path[VALUE_MAX];
  int fd, byte;

  // by koh TODO: Enter the duty_cycle path.
  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm0/duty_cycle", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open in duty cycle!\n");
    return (-1);
  }
  byte = snprintf(s_value_str, VALUE_MAX, "%d", value);

  if (-1 == write(fd, s_value_str, byte)) {
    fprintf(stderr, "Failed to write value in duty cycle!\n");
    close(fd);
    return -1;
  }
  close(fd);

  return (0);
}

