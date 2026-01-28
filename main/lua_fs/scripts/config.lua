-- config.lua
-- Mapa de pines GPIO y valores de configuración usados por los scripts Lua
-- Ajusta estos valores según tu hardware y particionado de pines.

local config = {}

-- Pines mapeados según components/lhos_config/lhos_config.h
config.pins = {
  LED_GPIO = 48,          -- LHOS_GPIO_LED
  WS2812_DATA = 48,       -- WS2812B data pin (same as LED)
  UART1_TX = 39,          -- LHOS_GPIO_RS232_1_TX
  UART1_RX = 40,          -- LHOS_GPIO_RS232_1_RX
  UART2_TX = 41,          -- LHOS_GPIO_RS232_2_TX
  UART2_RX = 42,          -- LHOS_GPIO_RS232_2_RX
  I2C_SDA = 8,            -- LHOS_GPIO_I2C_SDA
  I2C_SCL = 9,            -- LHOS_GPIO_I2C_SCL
  SPI_CS = 10,            -- LHOS_GPIO_CS
  SPI_MOSI = 11,          -- LHOS_GPIO_SPI_MOSI
  SPI_SCK = 12,           -- LHOS_GPIO_SPI_SCK
  SPI_MISO = 13,          -- LHOS_GPIO_SPI_MISO
  SD_CS = 14,             -- LHOS_GPIO_SD_CS
  RS485_TX = 17,          -- LHOS_GPIO_RS485_TX
  RS485_RX = 18,          -- LHOS_GPIO_RS485_RX
  RS485_RE_DE = 21,       -- LHOS_GPIO_RS485_RE_DE
  USB_DN = 19,            -- LHOS_GPIO_USB_DN
  USB_DP = 20,            -- LHOS_GPIO_USB_DP
}

-- Parámetros de comportamiento
config.led = {
  ws2812 = true,          -- true si se usa tira WS2812b en vez de GPIO simple
  default_color = { r = 0, g = 128, b = 0 },
  -- Colores en RGB24 (0xRRGGBB) para uso sencillo desde scripts
  colors = {
    ok = 0x00C800,      -- verde suave
    partial = 0xC85000, -- ámbar
    fail = 0xC80000,    -- rojo
  },
  -- Paleta de 16 colores (RGB24) - estilo paleta ANSI básica
  palette = {
    0x000000, -- 0 Black
    0x800000, -- 1 Red
    0x008000, -- 2 Green
    0x808000, -- 3 Yellow
    0x000080, -- 4 Blue
    0x800080, -- 5 Magenta
    0x008080, -- 6 Cyan
    0xC0C0C0, -- 7 White
    0x808080, -- 8 Bright Black (Gray)
    0xFF0000, -- 9 Bright Red
    0x00FF00, -- 10 Bright Green
    0xFFFF00, -- 11 Bright Yellow
    0x0000FF, -- 12 Bright Blue
    0xFF00FF, -- 13 Bright Magenta
    0x00FFFF, -- 14 Bright Cyan
    0xFFFFFF, -- 15 Bright White
  },
  -- Constantes con nombres para acceder a la paleta (1-based index)
  PAL = {
    BLACK = 1,
    RED = 2,
    GREEN = 3,
    YELLOW = 4,
    BLUE = 5,
    MAGENTA = 6,
    CYAN = 7,
    WHITE = 8,
    B_BLACK = 9,
    B_RED = 10,
    B_GREEN = 11,
    B_YELLOW = 12,
    B_BLUE = 13,
    B_MAGENTA = 14,
    B_CYAN = 15,
    B_WHITE = 16,
  },
  fade_ms = 500,
}

config.uart = {
  baud = 115200,
  rx_buffer = 256,
}

-- Parámetros por puerto UART (0..2)
-- Cada entrada puede usarse directamente para llamar a `uart.open()`:
-- e.g. uart.open(1, cfg.uart[1].baud, cfg.uart[1].tx_pin, cfg.uart[1].rx_pin, { parity=..., flow_control=..., rx_buffer_size=... })
config.uart_ports = {
  [0] = {
    baud = 115200,
    parity = "none",       -- "none" | "even" | "odd"
    flow_control = false,
    tx_pin = nil,           -- console pins: leave nil to use default
    rx_pin = nil,
    rx_buffer_size = 1024,
    tx_buffer_size = 0,
    data_bits = 8,         -- 5..8
    stop_bits = 1,         -- 1 or 2
    -- RS-485 / DE (half-duplex) options
    de_pin = nil,          -- if set, GPIO pin used for DE (driver enable)
    de_active_high = true, -- whether DE is active high
    tx_delay_ms = 1,       -- delay after asserting DE before starting TX
    turn_off_delay_ms = 2, -- delay after TX done before releasing DE
  },
  [1] = {
    baud = 9600,
    parity = "none",
    flow_control = false,
    tx_pin = config.pins.UART1_TX,            -- LHOS_GPIO_RS232_1_TX
    rx_pin = config.pins.UART1_RX,            -- LHOS_GPIO_RS232_1_RX
    rx_buffer_size = 2048,
    tx_buffer_size = 0,
    data_bits = 8,
    stop_bits = 1,
    de_pin = config.pins.UART1_DE,
    de_active_high = true,
    tx_delay_ms = 1,
    turn_off_delay_ms = 2,
  },
  [2] = {
    baud = 9600,
    parity = "none",
    flow_control = false,
    tx_pin = config.pins.UART2_TX,            -- LHOS_GPIO_RS232_2_TX
    rx_pin = config.pins.UART2_RX,            -- LHOS_GPIO_RS232_2_RX
    rx_buffer_size = 2048,
    tx_buffer_size = 0,
    data_bits = 8,
    stop_bits = 1,
    de_pin = config.pins.UART2_DE,
    de_active_high = true,
    tx_delay_ms = 1,
    turn_off_delay_ms = 2,
  },
}

-- WiFi configuration (fill with your SSID/password). Avoid committing secrets to
-- shared repositories; consider loading from a secure store for production.
config.wifi = {
  mode = "sta",          -- "sta" | "ap" | "sta+ap"
  ssid = "LinuxAcademy",    -- replace with your network SSID
  password = "m@st3rk3y*12255221!!1234567890**",-- replace with your network password
  auto_connect = true,
  ap = {                  -- options used when mode includes AP
    ssid = "lhOS_AP",
    password = "lhOS_pass",
    channel = 1,
    max_connections = 4,
  },
}

-- Export
return config
