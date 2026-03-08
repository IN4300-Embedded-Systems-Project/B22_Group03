/* =============================================================================
 * Forest Acoustic Sentinel — Main Firmware
 * ESP32-S3 + INMP441 + LoRa SX1278
 *
 * Detects illegal chainsaw and mining activity using edge ML inference
 * and transmits alerts via LoRa 433MHz.
 * ========================================================================== */

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);  // Wait up to 3s for Serial

    Serial.println();
    Serial.println("========================================");
    Serial.println("  Forest Acoustic Sentinel v1.0");
    Serial.println("  ESP32-S3 + INMP441 + LoRa SX1278");
    Serial.println("========================================");
    Serial.println();
}

// ---------------------------------------------------------------------------
// Main Loop
// ---------------------------------------------------------------------------
void loop() {
    delay(1000);
}
