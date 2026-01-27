#ifndef LHOS_CONFIG_H
#define LHOS_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

void lhos_config_init (void);
bool lhos_config_lua_enabled (void);
bool lhos_config_lua_use_spiram (void);
const char *lhos_config_target (void);

#define LHOS_GPIO_IO1 1          // GPIO1 - J5.I/O-10 (IO1)
#define LHOS_GPIO_IO2 2          // GPIO2 - J5.I/O-9  (IO2)
#define LHOS_GPIO_IO3 3          // GPIO3 - J6.COM-4 (K-POWER(IO3))
#define LHOS_GPIO_IO4 4          // GPIO4 - J6.COM-7 (SLEEP (IO4))
#define LHOS_GPIO_IO5 5          // GPIO5 - J7.I/O-6 (IO5, ACC INT2)
#define LHOS_GPIO_IO6 6          // GPIO6 - J7.I/O-5 (IO6, ACC INT1)
#define LHOS_GPIO_IO7 7          // GPIO7 - J7.I/O-4 (IO7, MAG INT)
#define LHOS_GPIO_IO19 19        // GPIO19 - J8.SPI-5 (IO19, USB_DN)
#define LHOS_GPIO_IO20 20        // GPIO20 - J8.SPI-7 (IO20, USB_DP)
#define LHOS_GPIO_IO41 41        // GPIO41 - J5.I/O-2 (IO41, RS232_2 TX)
#define LHOS_GPIO_IO42 42        // GPIO42 - J5.I/O-1 (IO42, RS232_2 RX)
#define LHOS_GPIO_IO14 14        // GPIO14 - J7.I/O-3 (IO14, MAG DRDY)
#define LHOS_GPIO_IO46 46        // GPIO46 - J7.I/O-2 (IO46)
#define LHOS_GPIO_IO47 47        // GPIO47 - J7.I/O-1 (IO47)
#define LHOS_GPIO_I2C_SDA 8      // I2C SDA - J9.I2C-2
#define LHOS_GPIO_I2C_SCL 9      // I2C SCL - J9.I2C-1
#define LHOS_GPIO_CS 10          // SPI CS - J8.SPI-6
#define LHOS_GPIO_SPI_MOSI 11    // SPI MOSI - J8.SPI-3
#define LHOS_GPIO_SPI_SCK 12     // SPI SCK - J8.SPI-2
#define LHOS_GPIO_SPI_MISO 13    // SPI MISO - J8.SPI-1
#define LHOS_GPIO_SD_CS 14       // SD Card CS - J8.SPI-8
#define LHOS_GPIO_IO45 45        // GPIO45 - J8.SPI-4
#define LHOS_GPIO_USB_DN 19      // USB D- pin
#define LHOS_GPIO_USB_DP 20      // USB D+ pin
#define LHOS_GPIO_COMM_RX 15     // HUB Rx - J6.COM-3
#define LHOS_GPIO_COMM_TX 16     // HUB Tx - J6.COM-4
#define LHOS_GPIO_RS485_RX 18    // RS485 Rx pin
#define LHOS_GPIO_RS485_TX 17    // RS485 Tx pin
#define LHOS_GPIO_RS485_RE_DE 21 // RS485 RE/DE pin
#define LHOS_GPIO_RS232_1_RX 40  // RS232_1 Rx - J5.I/O-5
#define LHOS_GPIO_RS232_1_TX 39  // RS232_1 Tx - J5.I/O-6
#define LHOS_GPIO_RS232_2_RX 42  // RS232_2 Rx - J5.I/O-3
#define LHOS_GPIO_RS232_2_TX 41  // RS232_2 Tx - J5.I/O-4
#define LHOS_GPIO_LED 48         // RGB LED PORT

// LED color definitions
typedef enum
{
  LHOS_LED_OFF = 0,
  LHOS_LED_RED,
  LHOS_LED_GREEN,
  LHOS_LED_BLUE,
  LHOS_LED_WHITE,
  LHOS_LED_YELLOW,
  LHOS_LED_CYAN,
  LHOS_LED_MAGENTA
} lhos_led_color_t;

// Color table for RGB LED (common anode: 0=on, 1=off)
typedef struct
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
} lhos_led_rgb_t;

extern const lhos_led_rgb_t lhos_led_color_table[];

#endif /* LHOS_CONFIG_H */
