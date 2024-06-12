#include "JaeYukBookkom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "LCD.h"

LCDDevice lcd1, lcd2, lcd_error;

int LCD_init(void){
    LCDDevice **initlist = malloc(LCD_COUNT * sizeof(LCDDevice *));
    initlist[0] = &lcd1, initlist[1] = &lcd2, initlist[2] = &lcd_error;

    if (LCD_open(&lcd1, 0x23, BUS_NUMBER) < 0 || LCD_open(&lcd2, 0x26, BUS_NUMBER) < 0 || LCD_open(&lcd_error, 0x27, BUS_NUMBER) < 0)
    {
        return -1;
    }
    for (int i = 0; i < LCD_COUNT; i++)
    {
        LCD_write(initlist[i], 0x03, 0);
        LCD_write(initlist[i], 0x03, 0);
        LCD_write(initlist[i], 0x03, 0);
        LCD_write(initlist[i], 0x02, 0);
        LCD_write(initlist[i], LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS | LCD_4BITMODE, 0);
        LCD_write(initlist[i], LCD_DISPLAYCONTROL | LCD_DISPLAYON, 0);
        LCD_write(initlist[i], LCD_CLEARDISPLAY, 0);
        LCD_write(initlist[i], LCD_ENTRYMODESET | LCD_ENTRYLEFT, 0);
        LCD_clear(initlist[i]);
        LCD_display_string(initlist[i], "S      A      T ", 1);
        LCD_display_string(initlist[i], "    T     R     ", 2);
    }

    usleep(5000000);
    for (int i = 0; i < LCD_COUNT; i++)
    {
        LCD_clear(initlist[i]);
    }
    free(initlist);
}
void LCD_weather_prediction(float temp, float hyd, int weatherCode, int dust, int rainPer){
    LCD_clear(&lcd1);
    char str[16];
    snprintf(str, 16, "%.1f C, %.1f %", temp, hyd);
    LCD_display_string(&lcd1, str, 1);
/*
    Sunny = 0,
    Cloud,
    Windy,
    Rain,
    Snow
*/
    if (weatherCode == Sunny)
        snprintf(str, 16, "Sunny");
    else if (weatherCode == Cloud)
        snprintf(str, 16, "Cloud");
    else if (weatherCode == Windy)
        snprintf(str, 16, "Windy");
    else if (weatherCode == Rain)
        snprintf(str, 16, "Rain");
    else if (weatherCode == Snow)
        snprintf(str, 16, "Snow");
    LCD_display_string(&lcd1, str, 2);
    LCD_clear(&lcd2);
    if(dust > 100)
        snprintf(str, 16, "dust: BAD(%d)",dust);
    else
        snprintf(str, 16, "dust: GOOD(%d)",dust);
    LCD_display_string(&lcd2, str, 1);

    if(rainPer>60)
        snprintf(str,16,"!!!!!rainPer: %d",rainPer);
    else
        snprintf(str,16,"rainPer : %d",rainPer);
    LCD_display_string(&lcd2, str, 2);
}
void LCD_messaging_error(int errorcode){
    if (errorcode == 0)
    {
        LCD_clear(&lcd_error);
        LCD_display_string(&lcd_error, "Nothing Problem", 1);
        LCD_display_string(&lcd_error, "Outdoor is GOOD", 2);
    }
    else if (errorcode == 1)
    {
        LCD_clear(&lcd_error);
        LCD_display_string(&lcd_error, "Raining", 1);
        LCD_display_string(&lcd_error, "Outdoor is BAD", 2);
    }
    else if (errorcode == 2)
    {
        LCD_clear(&lcd_error);
        LCD_display_string(&lcd_error, "Gas", 1);
        LCD_display_string(&lcd_error, "Outdoor is BAD", 2);
    }
    else if (errorcode == 3)
    {
        LCD_clear(&lcd_error);
        LCD_display_string(&lcd_error, "Raining, Dust", 1);
        LCD_display_string(&lcd_error, "Outdoor is BAD", 2);
    }
    else if (errorcode == 4)
    {
        LCD_clear(&lcd_error);
        LCD_display_string(&lcd_error, "Noisy", 1);
        LCD_display_string(&lcd_error, "Outdoor is BAD", 2);
    }
    else if (errorcode == 5)
    {
        LCD_clear(&lcd_error);
        LCD_display_string(&lcd_error, "Raining, Noisy", 1);
        LCD_display_string(&lcd_error, "Outdoor is BAD", 2);
    }
    else if (errorcode == 6)
    {
        LCD_clear(&lcd_error);
        LCD_display_string(&lcd_error, "Dust, Noisy", 1);
        LCD_display_string(&lcd_error, "Outdoor is BAD", 2);
    }
    else if (errorcode == 7)
    {
        LCD_clear(&lcd_error);
        LCD_display_string(&lcd_error, "Rain,Dust,Noisy", 1);
        LCD_display_string(&lcd_error, "Outdoor is BAD", 2);
    }
    else if (errorcode == 8)
    {
        LCD_clear(&lcd_error);
        LCD_display_string(&lcd_error, "Danger", 1);
        LCD_display_string(&lcd_error, "Check Window", 2);
    }
}
int LCD_open(LCDDevice *dev, int addr, int bus){
    char filename[20];
    snprintf(filename, 19, "/dev/i2c-%d", bus);
    dev->file = open(filename, O_RDWR);
    if (dev->file < 0)
    {
        perror("Failed to open the i2c bus");
        return -1;
    }
    dev->addr = addr;
    dev->bus = bus;
    if (ioctl(dev->file, I2C_SLAVE, dev->addr) < 0)
    {
        perror("Failed to acquire bus access and/or talk to slave");
        return -1;
    }
    return 0;
}
void LCDdispose(void){
    close(lcd1.file);
    close(lcd2.file);
    close(lcd_error.file);
}
void i2c_write_cmd(LCDDevice *dev, uint8_t cmd){
    if (write(dev->file, &cmd, 1) != 1)
        perror("Failed to write command to the i2c bus");
    usleep(100);
}
void LCD_strobe_signal(LCDDevice *dev, uint8_t data){ // strobe signal: notify send data
    i2c_write_cmd(dev, data | En | LCD_BACKLIGHT);
    usleep(500);
    i2c_write_cmd(dev, ((data & ~En) | LCD_BACKLIGHT));
    usleep(100);
}
void LCD_write(LCDDevice *dev, uint8_t cmd, uint8_t mode){
    i2c_write_cmd(dev, mode | (cmd & 0xF0) | LCD_BACKLIGHT);
    LCD_strobe_signal(dev, mode | (cmd & 0xF0));
    i2c_write_cmd(dev, mode | ((cmd << 4) & 0xF0) | LCD_BACKLIGHT);
    LCD_strobe_signal(dev, mode | ((cmd << 4) & 0xF0));
}
void LCD_display_string(LCDDevice *dev, const char *string, int line){
    if(line == 1) LCD_write(dev, 0x80, 0);
    else if(line == 2) LCD_write(dev, 0xC0, 0);
    
    while (*string){
        LCD_write(dev, *string++, Rs);
    }
}
void LCD_clear(LCDDevice *dev) //clear display
{
    LCD_write(dev, LCD_CLEARDISPLAY, 0);
    LCD_write(dev, LCD_RETURNHOME, 0);
}

