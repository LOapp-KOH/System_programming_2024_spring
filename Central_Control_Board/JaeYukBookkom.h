#ifndef __JaeYukBookkom__
#define __JaeYukBookkom__
#define MAX_CONNECT_BOARD 3
#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define BUFFER_MAX 3
#define DIRECTION_MAX 256
#define VALUE_MAX 256
// GPIO PIN Settings

#define BUTTON_LED 26
#define BUTTON_OUT 19
#define BUTTON_IN 13
#define TIMER_OUT 27
#define TIMER_IN 22

#define SPEAKER 0 // PWM 0

enum WeatherCast
{
    Sunny = 0,
    Cloud,
    Windy,
    Rain,
    Snow
};
#endif
