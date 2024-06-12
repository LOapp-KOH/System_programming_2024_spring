// LCD Instruction
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// Display mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// Display on/off Control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// Display Cursor Movement
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// Mode Setting
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_5x8DOTS 0x00

// Using BackLight
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

// GPIO Bit Settings
#define En 0b00000100 // Enable
#define Rw 0b00000010 // ReadWrite
#define Rs 0b00000001 // Register Select

//Number of LCD Setting
#define LCD_COUNT 3
// I2C Bus Number Setting
#define BUS_NUMBER 1

typedef struct {
    int addr;
    int bus;
    int file;
} LCDDevice;

int LCD_init(void);
void LCD_weather_prediction(float temp, float hyd, int weatherCode, int dust, int rainPer);
void LCD_messaging_error(int errorcode);
int LCD_open(LCDDevice *dev, int addr, int bus);
void LCDdispose(void);
void i2c_write_cmd(LCDDevice *dev, uint8_t cmd);
void LCD_strobe_signal(LCDDevice *dev, uint8_t data);
void LCD_write(LCDDevice *dev, uint8_t cmd, uint8_t mode);
void LCD_display_string(LCDDevice *dev, const char *string, int line);
void LCD_clear(LCDDevice *dev);