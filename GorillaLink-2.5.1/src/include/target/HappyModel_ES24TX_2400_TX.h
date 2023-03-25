#ifndef DEVICE_NAME
#define DEVICE_NAME "HM ES24TX"
#endif

// Any device features
#define USE_TX_BACKPACK
#define USE_SX1280_DCDC

// GPIO pin definitions
#define GPIO_PIN_NSS            5
#define GPIO_PIN_BUSY           21
#define GPIO_PIN_DIO1           4
#define GPIO_PIN_MOSI           23
#define GPIO_PIN_MISO           19
#define GPIO_PIN_SCK            18
#define GPIO_PIN_RST            14
#define GPIO_PIN_RX_ENABLE      27
#define GPIO_PIN_TX_ENABLE      26
#define GPIO_PIN_RCSIGNAL_RX    13
#define GPIO_PIN_RCSIGNAL_TX    13
#define GPIO_PIN_LED_WS2812     15
#define GPIO_PIN_FAN_EN         17

// Output Power
#define MinPower PWR_10mW
#define MaxPower PWR_250mW
#define POWER_OUTPUT_VALUES {-17,-13,-9,-6,-2}

#define Regulatory_Domain_ISM_2400 1
