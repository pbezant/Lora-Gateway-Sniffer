#ifndef CONFIG_H
#define CONFIG_H

// =====================
// Heltec Wireless Tracker v1.1 (ESP32-S3FN8) Pin Mapping
// =====================

// --- LoRa SX1262 (Heltec v1.1 Official) ---
#define LORA_CS     8    // LoRa_NSS
#define LORA_RST    12   // LoRa_RST
#define LORA_DIO1   14   // LoRa_DIO1
#define LORA_BUSY   13   // LoRa_Busy
#define LORA_SCK    9    // LoRa_SCK (FSPI SCK)
#define LORA_MISO   11   // LoRa_MISO (FSPI MISO)
#define LORA_MOSI   10   // LoRa_MOSI (FSPI MOSI)

// --- Display (ST7735 160x80) ---
#define TFT_CS      38   // Chip Select
#define TFT_RST     39   // Reset
#define TFT_DC      40   // Data/Command
#define TFT_SCLK    41   // SPI Clock
#define TFT_MOSI    42   // SPI Data
#define TFT_BLK     21   // Backlight

// --- GPS (UC6580) ---
#define GPS_TX_PIN      33  // GPS TX (to ESP RX)
#define GPS_RX_PIN      34  // GPS RX (to ESP TX)
#define GPS_PWR_PIN     3   // GPS Power Enable (HIGH = ON)
#define GPS_BAUD_RATE   9600

// --- Power Control ---
#define VEXT_PIN    1   // External Power Enable (HIGH = ON)
#define VTFT_PIN    2   // Display Power Enable (HIGH = ON)
#define VGNSS_PIN   3   // GPS Power Enable (HIGH = ON)

// --- Battery ---
#define BATTERY_PIN 15  // Analog input for battery voltage

// --- LoRaWAN Timing ---
#define LORA_SEND_INTERVAL 60000    // Send data every 60 seconds
#define LORA_JOIN_TIMEOUT  30000    // Join timeout in milliseconds
#define LORA_RETRY_DELAY   10000    // Retry delay between join attempts

// --- User Button & LED ---
#define USER_BUTTON_PIN 0   // User button (GPIO0)
#define USER_LED_PIN    35  // User LED (GPIO35)

#endif // CONFIG_H 