/* =============================================================================
 * lora_handler.cpp - LoRa SX1278 Communication Implementation
 *
 * Manages the SX1278 (Ra-02 433MHz) radio module via SPI.
 * Uses the Sandeep Mistry LoRa library for packet transmission.
 *
 * Packet format (comma-separated string):
 *   Alert:     NODE_ID,alert,ALERT_TYPE,CONFIDENCE,COUNT,TIMESTAMP,LAT,LNG
 *   Heartbeat: NODE_ID,data,ambient,0.00,0,TIMESTAMP,LAT,LNG
 * ========================================================================== */

#include "lora_handler.h"
#include "config.h"

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
static unsigned long lastTxTime = 0;   // Timestamp of last transmission
static bool          radioReady = false;

// ---------------------------------------------------------------------------
// Initialize LoRa radio - configure SPI pins and radio parameters
// ---------------------------------------------------------------------------
bool lora_init(void) {
    // Configure SPI pins for LoRa module
    // ESP32-S3 allows remapping SPI to any GPIO
    SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_NSS_PIN);

    // Set LoRa module control pins
    LoRa.setPins(LORA_NSS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);

    // Initialize LoRa at configured frequency
    if (!LoRa.begin(LORA_FREQUENCY)) {
        Serial.println("[LORA] ERROR: LoRa initialization failed!");
        Serial.println("[LORA] Check wiring: SCK=18, MISO=19, MOSI=23, NSS=5, RST=14, DIO0=26");
        radioReady = false;
        return false;
    }

    // Configure radio parameters for range vs power tradeoff
    LoRa.setTxPower(LORA_TX_POWER);            // Transmit power in dBm
    LoRa.setSpreadingFactor(LORA_SPREADING);    // Spreading factor (7-12)
    LoRa.setSignalBandwidth(LORA_BANDWIDTH);    // Bandwidth in Hz
    LoRa.setCodingRate4(LORA_CODING_RATE);      // Coding rate denominator (5-8)
    LoRa.enableCrc();                           // Enable CRC for error detection

    // Put radio to sleep immediately - only wake for TX
    LoRa.sleep();

    radioReady = true;
    Serial.printf("[LORA] Initialized - %.0f MHz, TX %ddBm, SF%d, BW %.0fkHz\n",
                  LORA_FREQUENCY / 1E6, LORA_TX_POWER, LORA_SPREADING,
                  LORA_BANDWIDTH / 1E3);
    return true;
}

// ---------------------------------------------------------------------------
// Send an alert packet via LoRa
// ---------------------------------------------------------------------------
bool lora_send_alert(const char* alertType, float confidence,
                     int consecutiveCount, unsigned long timestampMs) {
    if (!radioReady) {
        Serial.println("[LORA] ERROR: Radio not initialized");
        return false;
    }

    // Wake radio from sleep for transmission
    LoRa.idle();

    // Build comma-separated packet string
    // Format: NODE_ID,alert,ALERT_TYPE,CONFIDENCE,COUNT,TIMESTAMP,LAT,LNG
    char packet[128];
    snprintf(packet, sizeof(packet), "%s,alert,%s,%.2f,%d,%lu,%.4f,%.4f",
             NODE_ID, alertType, confidence, consecutiveCount,
             timestampMs, NODE_LAT, NODE_LNG);

    // Transmit packet
    LoRa.beginPacket();
    LoRa.print(packet);
    int txResult = LoRa.endPacket(true);  // true = async (non-blocking) TX

    if (txResult == 0) {
        Serial.println("[LORA] WARNING: endPacket returned 0 (possible TX issue)");
    }

    // Record transmission time for cooldown tracking
    lastTxTime = millis();

    // Return radio to sleep after TX
    LoRa.sleep();

    Serial.printf("[LORA] TX Alert: %s\n", packet);
    return true;
}

// ---------------------------------------------------------------------------
// Send a heartbeat/data packet via LoRa
// ---------------------------------------------------------------------------
bool lora_send_heartbeat(void) {
    if (!radioReady) {
        Serial.println("[LORA] ERROR: Radio not initialized");
        return false;
    }

    // Wake radio from sleep
    LoRa.idle();

    // Build heartbeat packet
    // Format: NODE_ID,data,ambient,0.00,0,TIMESTAMP,LAT,LNG
    char packet[128];
    snprintf(packet, sizeof(packet), "%s,data,ambient,0.00,0,%lu,%.4f,%.4f",
             NODE_ID, millis(), NODE_LAT, NODE_LNG);

    // Transmit packet
    LoRa.beginPacket();
    LoRa.print(packet);
    int txResult = LoRa.endPacket(true);

    if (txResult == 0) {
        Serial.println("[LORA] WARNING: endPacket returned 0 (possible TX issue)");
    }

    // Record transmission time
    lastTxTime = millis();

    // Return radio to sleep
    LoRa.sleep();

    Serial.printf("[LORA] TX Heartbeat: %s\n", packet);
    return true;
}

// ---------------------------------------------------------------------------
// Check if cooldown is active - prevents rapid-fire transmissions
// ---------------------------------------------------------------------------
bool lora_is_cooldown_active(void) {
    if (lastTxTime == 0) return false;  // Never transmitted yet
    return (millis() - lastTxTime) < ALERT_COOLDOWN_MS;
}

// ---------------------------------------------------------------------------
// Put LoRa radio into low-power sleep mode
// ---------------------------------------------------------------------------
void lora_sleep(void) {
    if (radioReady) {
        LoRa.sleep();
    }
}

// ---------------------------------------------------------------------------
// Wake LoRa radio from sleep to idle/standby mode
// ---------------------------------------------------------------------------
void lora_wake(void) {
    if (radioReady) {
        LoRa.idle();
    }
}
