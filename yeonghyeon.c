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
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <signal.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define BUFFER_MAX 3
#define DIRECTION_MAX 256
#define VALUE_MAX 256

// GPIO 핀 정의
#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

// 우적 센서의 GPIO 핀 번호
#define RAIN_SENSOR_PIN 16

// 가스와 소음 센서의 임계값 설정
#define GAS_THRESHOLD 500
#define SOUND_THRESHOLD 30

// SPI 설정
static const char *DEVICE = "/dev/spidev0.0"; // SPI 장치 파일 경로
static uint8_t MODE = 0;
static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000; // 1MHz
static uint16_t DELAY = 5;
static int SAMPLE_SIZE = 10; // 소음 센서 평균을 계산할 샘플 크기

// 서버 정보
const char* SERVER_IP = "192.168.22.4"; // 서버 IP
const int SERVER_PORT = 8800; // 서버 포트

// 전역 소켓 변수
int sock;
int spi_fd;

// 전역 상태 변수
int rain_status = 0;
int gas_status = 0;
int sound_status = 0;
int rain_initialized = 0; // 초기 상태 전송 플래그
int gas_initialized = 0; // 초기 상태 전송 플래그
int sound_initialized = 0; // 초기 상태 전송 플래그
pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER; // 상태 변수 보호용 뮤텍스

// GPIO 함수 선언
static int GPIOExport(int pin);
static int GPIOUnexport(int pin);
static int GPIODirection(int pin, int dir);
static int GPIORead(int pin);

// 센서 스레드 함수 선언
void* rain_sensor_routine(void* arg);
void* gas_sensor_routine(void* arg);
void* sound_sensor_routine(void* arg);

// 클라이언트 소켓 함수 선언
void setup_socket();
void send_message_to_server(int value);
void dispose();
void update_status_and_send_message();

// 인터럽트 핸들러 함수
void handle_interrupt(int sig) {
    dispose();
    exit(0);
}

// GPIO 핀을 시스템에 export 위한 함수
static int GPIOExport(int pin) {
    char buffer[3];
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd == -1) {
        perror("Failed to open export for writing");
        return -1;
    }
    snprintf(buffer, 3, "%d", pin);
    if (write(fd, buffer, 3) == -1) {
        perror("Failed to export gpio");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

// GPIO 핀을 시스템에서 unexport 위한 함수
static int GPIOUnexport(int pin) {
    char buffer[3];
    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd == -1) {
        perror("Failed to open unexport for writing");
        return -1;
    }
    snprintf(buffer, 3, "%d", pin);
    if (write(fd, buffer, 3) == -1) {
        perror("Failed to unexport gpio");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

// GPIO 핀의 방향을 설정하는 함수
static int GPIODirection(int pin, int dir) {
    const char s_directions_str[] = "in\0out";
    char path[40];
    int fd;

    snprintf(path, 40, "/sys/class/gpio/gpio%d/direction", pin);
    fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("Failed to open gpio direction for writing");
        return -1;
    }

    if (write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3) == -1) {
        perror("Failed to set direction");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

// GPIO 핀의 값을 읽어오는 함수
static int GPIORead(int pin) {
    char path[40];
    char value_str[3];
    int fd;

    snprintf(path, 40, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open gpio value for reading");
        return -1;
    }

    if (read(fd, value_str, 3) == -1) {
        perror("Failed to read value");
        close(fd);
        return -1;
    }
    close(fd);

    return atoi(value_str);
}

// 클라이언트 소켓 설정 함수
void setup_socket() {
    struct sockaddr_in serv_addr;
    
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket() error");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serv_addr.sin_port = htons(SERVER_PORT);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("connect() error");
        close(sock);
        exit(1);
    }

    printf("Connection established\n");
}

// 클라이언트 소켓으로 서버에 메시지를 보내는 함수
void send_message_to_server(int value) {
    char output_str[3] = {0};
    snprintf(output_str, sizeof(output_str), "%d", value);
    int len = write(sock, output_str, strlen(output_str));
    if (len == -1) {
        perror("write() error");
    } else {
        printf("Sent message to server: %s\n", output_str);
    }
}

// 자원 해제 함수. 강제 종료했을 때, 리소스 오류를 방지
void dispose() {
    GPIOUnexport(RAIN_SENSOR_PIN);
    close(sock);
    close(spi_fd);
    pthread_mutex_destroy(&status_mutex);
    printf("Resources disposed\n");
}

// 상태 업데이트 및 메시지 전송 함수
void update_status_and_send_message() {
    pthread_mutex_lock(&status_mutex); // 상태 변수 보호를 위해 뮤텍스 사용
    if (rain_initialized && gas_initialized && sound_initialized) {
        int combined_status = rain_status + gas_status + sound_status;
        send_message_to_server(combined_status);
    }
    pthread_mutex_unlock(&status_mutex);
}

// 우적 센서 스레드 함수
void* rain_sensor_routine(void* arg) {
    int previous_state = -1; // 이전 상태를 저장하기 위한 변수
    while (1) {
        int value = GPIORead(RAIN_SENSOR_PIN);
        if (value == -1) {
            printf("Failed to read from rain sensor\n");
            break;
        }
        int current_state = (value == LOW) ? 1 : 0; // 비 감지 시 1, 비 없음 시 0
        if (current_state == 1) {
            printf("빗물 감지\n");
        }
        if (current_state != previous_state) {
            pthread_mutex_lock(&status_mutex); // 상태 변수 보호를 위해 뮤텍스 사용
            rain_status = current_state == 1 ? 1 : 0;
            rain_initialized = 1;
            pthread_mutex_unlock(&status_mutex);
            update_status_and_send_message();
            previous_state = current_state;
        }
        usleep(1000000); // 1초 대기
    }
    return NULL;
}

// SPI 준비 함수
static int prepare(int fd) {
    if (ioctl(fd, SPI_IOC_WR_MODE, &MODE) == -1) {
        perror("Can't set MODE");
        return -1;
    }

    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS) == -1) {
        perror("Can't set number of BITS");
        return -1;
    }

    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &CLOCK) == -1) {
        perror("Can't set write CLOCK");
        return -1;
    }

    if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &CLOCK) == -1) {
        perror("Can't set read CLOCK");
        return -1;
    }

    return 0;
}

// 차동 모드용 컨트롤 비트를 생성하는 함수
uint8_t control_bits_differential(uint8_t channel) {
    return (channel & 7) << 4;
}

// 단일 모드용 컨트롤 비트를 생성하는 함수
uint8_t control_bits(uint8_t channel) {
    return 0x8 | control_bits_differential(channel);
}

// ADC 값을 읽어오는 함수
int readadc(int fd, uint8_t channel) {
    uint8_t tx[] = {1, control_bits(channel), 0};
    uint8_t rx[3];

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = ARRAY_SIZE(tx),
        .delay_usecs = DELAY,
        .speed_hz = CLOCK,
        .bits_per_word = BITS,
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1) {
        perror("IO Error");
        abort();
    }

    return ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);
}

// 가스 센서 스레드 함수
void* gas_sensor_routine(void* arg) {
    int fd = *(int*)arg;
    int previous_state = -1; // 이전 상태를 저장하기 위한 변수

    while (1) {
        int value = readadc(fd, 0); // 채널 0 사용
        printf("가스 센서 값: %d\n", value);

        int current_state = (value > GAS_THRESHOLD) ? 1 : 0; // 가스 감지 시 1, 가스 없음 시 0
        if (current_state != previous_state) {
            pthread_mutex_lock(&status_mutex); // 상태 변수 보호를 위해 뮤텍스 사용
            gas_status = current_state == 1 ? 2 : 0;
            gas_initialized = 1;
            pthread_mutex_unlock(&status_mutex);
            update_status_and_send_message();
            previous_state = current_state;
        }

        usleep(1000000); // 1초 대기
    }

    return NULL;
}

// 소음 센서 스레드 함수
void* sound_sensor_routine(void* arg) {
    int fd = *(int*)arg;
    int32_t samples[SAMPLE_SIZE];
    int sample_index = 0;
    int previous_state = -1; // 이전 상태를 저장하기 위한 변수

    while (1) {
        int value = readadc(fd, 7); // 채널 7 사용
        samples[sample_index] = value;
        sample_index = (sample_index + 1) % SAMPLE_SIZE;

        if (sample_index == 0) {
            int32_t sum = 0;
            for (int i = 0; i < SAMPLE_SIZE; i++) {
                sum += samples[i];
            }
            int32_t average_value = sum / SAMPLE_SIZE;
            printf("소음 센서 평균 값: %d\n", average_value);

            int current_state = (average_value > SOUND_THRESHOLD) ? 1 : 0; // 소음 감지 시 1, 소음 없음 시 0
            if (current_state != previous_state) {
                pthread_mutex_lock(&status_mutex); // 상태 변수 보호를 위해 뮤텍스 사용
                sound_status = current_state == 1 ? 4 : 0;
                sound_initialized = 1;
                pthread_mutex_unlock(&status_mutex);
                update_status_and_send_message();
                previous_state = current_state;
            }
        }

        usleep(1000000); // 1초 대기
    }

    return NULL;
}

int main(int argc, char **argv) {
    // 인터럽트 신호 처리 설정
    signal(SIGINT, handle_interrupt);
    signal(SIGTERM, handle_interrupt);

    // SPI 장치 열기
    spi_fd = open(DEVICE, O_RDWR);
    if (spi_fd <= 0) {
        perror("Device open error");
        return -1;
    }

    // SPI 장치 준비
    if (prepare(spi_fd) == -1) {
        perror("Device prepare error");
        return -1;
    }

    // 소켓 설정
    setup_socket();

    pthread_t rain_thread, gas_thread, sound_thread;

    // GPIO 초기화
    if (GPIOExport(RAIN_SENSOR_PIN) == -1) {
        return 1;
    }
    if (GPIODirection(RAIN_SENSOR_PIN, IN) == -1) {
        return 1;
    }

    printf("프로그램이 시작되었습니다. 센싱을 시작하기 전에 초기 설정을 완료합니다.\n");
    printf("10초 후에 센싱을 시작합니다. 잠시 후에 초기 값이 출력이 됩니다.\n");
    printf("먼저 소음 센서부터 센싱을 시작하며, 9초 후에 가스 센서와 우적 센서가 순차적으로 시작됩니다.\n");
    printf("센서 상태 코드: 정상 상태: 0, 우적 감지: 1, 가스 감지: 2, 소음 감지: 4\n");
    printf("복합 상태 코드: 우적 + 가스: 3, 우적 + 소음: 5, 가스 + 소음: 6, 우적 + 가스 + 소음: 7\n");
    printf("[초기 상태]\n");

    // 소음 센서 스레드 생성
    if (pthread_create(&sound_thread, NULL,sound_sensor_routine, &spi_fd) != 0) {
        perror("Failed to create sound sensor thread");
        return 1;
    }

    usleep(9000000); // 9초 대기 
    // 우적 센서 스레드 생성
    if (pthread_create(&rain_thread, NULL, rain_sensor_routine, NULL) != 0) {
        perror("Failed to create rain sensor thread");
        return 1;
    }

    // 가스 센서 스레드 생성
    if (pthread_create(&gas_thread, NULL, gas_sensor_routine, &spi_fd) != 0) {
        perror("Failed to create gas sensor thread");
        return 1;
    }

    // 스레드 종료 대기
    pthread_join(rain_thread, NULL);
    pthread_join(gas_thread, NULL);
    pthread_join(sound_thread, NULL);

    // 자원 해제
    dispose();

    return 0;
}
