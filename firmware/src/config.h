/* =============================================================================
 * config.h - SilentScout Configuration
 *
 * All pin definitions, thresholds, and constants in one place.
 * Modify these values to match your hardware wiring and deployment needs.
 * ========================================================================== */

#ifndef CONFIG_H
#define CONFIG_H

// ---------------------------------------------------------------------------
// Node Identity
// ---------------------------------------------------------------------------
#define NODE_ID         "N1"        // Unique identifier for this sentinel node
#define NODE_LAT        6.9270f     // Deployment latitude  (stub - no GPS module)
#define NODE_LNG        79.8610f    // Deployment longitude (stub - no GPS module)

// ---------------------------------------------------------------------------
// I2S Microphone - INMP441 Pin Mapping
// ---------------------------------------------------------------------------
#define I2S_WS_PIN      1           // Word Select  (LRCLK)
#define I2S_SD_PIN      2           // Serial Data   (DOUT)
#define I2S_SCK_PIN     42          // Serial Clock  (BCLK)
#define I2S_PORT        I2S_NUM_0   // I2S peripheral number

// ---------------------------------------------------------------------------
// Audio Sampling Parameters
// ---------------------------------------------------------------------------
#define SAMPLE_RATE         16000   // 16 kHz sample rate
#define BITS_PER_SAMPLE     16      // 16-bit audio
#define DMA_BUF_COUNT       8       // Number of DMA buffers
#define DMA_BUF_LEN         512     // Samples per DMA buffer

// ---------------------------------------------------------------------------
// LoRa SX1278 (Ra-02 433MHz) Pin Mapping - SPI
// ---------------------------------------------------------------------------
#define LORA_SCK_PIN    18          // SPI Clock
#define LORA_MISO_PIN   19          // SPI MISO
#define LORA_MOSI_PIN   23          // SPI MOSI
#define LORA_NSS_PIN    5           // Chip Select (NSS/CS)
#define LORA_RST_PIN    14          // Reset
#define LORA_DIO0_PIN   26          // DIO0 (TX/RX Done interrupt)

// ---------------------------------------------------------------------------
// LoRa Radio Parameters
// ---------------------------------------------------------------------------
#define LORA_FREQUENCY      433E6   // 433 MHz ISM band
#define LORA_TX_POWER       17      // dBm (max 20, use 17 for battery life)
#define LORA_SPREADING      7       // Spreading factor (7-12, lower = faster)
#define LORA_BANDWIDTH      125E3   // 125 kHz bandwidth
#define LORA_CODING_RATE    5       // 4/5 coding rate

// ---------------------------------------------------------------------------
// Detection Thresholds - False Positive Reduction
// ---------------------------------------------------------------------------
#define CONFIDENCE_THRESHOLD    0.85f   // Minimum confidence to count as valid
#define CONSECUTIVE_REQUIRED    3       // Same class N times before alert
#define ALERT_COOLDOWN_MS       10000   // 10s cooldown between LoRa transmissions

// ---------------------------------------------------------------------------
// Power Management
// ---------------------------------------------------------------------------
#define INFERENCE_SLEEP_MS      100         // Light sleep between inference cycles (ms)
#define LIGHT_SLEEP_DURATION_US 100000      // Light sleep duration in microseconds
#define HEARTBEAT_INTERVAL_MS   20000       // Send heartbeat every 20 seconds

// ---------------------------------------------------------------------------
// Serial Debug
// ---------------------------------------------------------------------------
#define SERIAL_BAUD_RATE    115200

#endif // CONFIG_H
