#ifndef DEVICE_NAME
#define DEVICE_NAME "ELRS 2400RX"
#endif

// GPIO pin definitions
#define GPIO_PIN_NSS         PA4
#define GPIO_PIN_MOSI        PA7
#define GPIO_PIN_MISO        PA6
#define GPIO_PIN_SCK         PA5

#define GPIO_PIN_DIO1        PA10
#define GPIO_PIN_RST         PB4
#define GPIO_PIN_BUSY        PA11

#define GPIO_PIN_RCSIGNAL_RX PB7  // USART1, AFAIO
#define GPIO_PIN_RCSIGNAL_TX PB6  // USART1, AFAIO

#define GPIO_PIN_LED_RED     PB5

// Output Power - use default SX1280

#define Regulatory_Domain_ISM_2400 1
