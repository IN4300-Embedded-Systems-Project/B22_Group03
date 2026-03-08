/* =============================================================================
 * Forest Acoustic Sentinel — Main Firmware
 * ESP32-S3 + INMP441 + LoRa SX1278
 *
 * Detects illegal chainsaw and mining activity using edge ML inference
 * and transmits alerts via LoRa 433MHz.
 * ========================================================================== */

#include <Arduino.h>
#include "config.h"

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial && millis() < 3000);  // Wait up to 3s for Serial

    Serial.println();
    Serial.println("========================================");
    Serial.println("  Forest Acoustic Sentinel v1.0");
    Serial.println("  ESP32-S3 + INMP441 + LoRa SX1278");
    Serial.println("========================================");
    Serial.printf("  Node ID    : %s\n", NODE_ID);
    Serial.printf("  Location   : %.4f, %.4f\n", NODE_LAT, NODE_LNG);
    Serial.printf("  Sample Rate: %d Hz\n", SAMPLE_RATE);
    Serial.printf("  LoRa Freq  : %.0f MHz\n", LORA_FREQUENCY / 1E6);
    Serial.printf("  Threshold  : %.0f%%  x%d consecutive\n",
                  CONFIDENCE_THRESHOLD * 100, CONSECUTIVE_REQUIRED);
    Serial.println("========================================");
    Serial.println();
}

// ---------------------------------------------------------------------------
// Main Loop
// ---------------------------------------------------------------------------
void loop() {
    delay(1000);
}
