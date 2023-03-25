#ifndef DEVICE_NAME
#define DEVICE_NAME "NM Voyager OLED"
#endif

// Features
#define USE_TX_BACKPACK
#define USE_OLED_SPI
#define WS2812_IS_GRB

// GPIO pin definitions
#define GPIO_PIN_RST            21
#define GPIO_PIN_DIO0           17
#define GPIO_PIN_DIO1           16
#define GPIO_PIN_NSS            5
#define GPIO_PIN_MOSI           23
#define GPIO_PIN_MISO           19
#define GPIO_PIN_SCK            18

/* DAC settings */
#define GPIO_PIN_RX_ENABLE      33
#define GPIO_PIN_RFamp_APC2     25

/* OLED definition */
#define GPIO_PIN_OLED_CS        12
#define GPIO_PIN_OLED_RST       27
#define GPIO_PIN_OLED_DC        26
#define GPIO_PIN_OLED_MOSI      15
#define GPIO_PIN_OLED_SCK       14

/* Joystick definition */
#define GPIO_PIN_JOYSTICK       35

/* S.Port input signal */
#define GPIO_PIN_RCSIGNAL_RX    13
#define GPIO_PIN_RCSIGNAL_TX    13
#define GPIO_PIN_FAN_EN         2

/* WS2812 led */
#define GPIO_PIN_LED_WS2812     4

// Output Power
#define POWER_OUTPUT_DACWRITE
#define MinPower                PWR_25mW
#define MaxPower                PWR_2000mW
#define POWER_OUTPUT_VALUES     {57,67,76,90,112,132,167,201}

/* Joystick values              {UP, DOWN, LEFT, RIGHT, ENTER, IDLE}*/
#define JOY_ADC_VALUES          {1850, 900, 490, 1427, 0, 2978}