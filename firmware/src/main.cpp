/* =============================================================================
 * Forest Acoustic Sentinel — Main Firmware
 * ESP32-S3 + INMP441 + LoRa SX1278
 *
 * Detects illegal chainsaw and mining activity using edge ML inference
 * and transmits alerts via LoRa 433MHz.
 * ========================================================================== */

#include <Arduino.h>
#include "config.h"
#include "audio_capture.h"
#include "lora_handler.h"

// ---------------------------------------------------------------------------
// Test audio buffer — allocated in PSRAM for large captures
// ---------------------------------------------------------------------------
#define TEST_CAPTURE_SAMPLES  16000  // 1 second of audio at 16kHz
static int16_t* audio_buffer = nullptr;

// ---------------------------------------------------------------------------
// Heartbeat tracking
// ---------------------------------------------------------------------------
static unsigned long lastHeartbeatTime = 0;

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

    // Initialize I2S microphone
    if (audio_init(SAMPLE_RATE) != 0) {
        Serial.println("[MAIN] FATAL: Audio init failed — halting");
        while (true) { delay(1000); }
    }

    // Allocate audio buffer in PSRAM (8MB PSRAM available on N16R8)
    audio_buffer = (int16_t*)ps_malloc(TEST_CAPTURE_SAMPLES * sizeof(int16_t));
    if (audio_buffer == nullptr) {
        Serial.println("[MAIN] FATAL: Failed to allocate audio buffer in PSRAM");
        while (true) { delay(1000); }
    }

    Serial.printf("[MAIN] Audio buffer allocated: %d samples (%d bytes) in PSRAM\n",
                  TEST_CAPTURE_SAMPLES, TEST_CAPTURE_SAMPLES * sizeof(int16_t));

    // Initialize LoRa radio
    if (!lora_init()) {
        Serial.println("[MAIN] WARNING: LoRa init failed — continuing without radio");
    } else {
        // Send initial heartbeat to announce presence
        lora_send_heartbeat();
        lastHeartbeatTime = millis();
    }

    Serial.println("[MAIN] Ready — capturing audio every 2 seconds...\n");
}

// ---------------------------------------------------------------------------
// Main Loop — Capture audio and print peak amplitude for testing
// ---------------------------------------------------------------------------
void loop() {
    Serial.println("[MAIN] Capturing audio...");

    // Fill the buffer with audio samples
    int result = audio_capture_buffer(audio_buffer, TEST_CAPTURE_SAMPLES);
    if (result != 0) {
        Serial.println("[MAIN] ERROR: Audio capture failed");
        delay(2000);
        return;
    }

    // Compute peak amplitude for mic verification
    int16_t peak = 0;
    for (int i = 0; i < TEST_CAPTURE_SAMPLES; i++) {
        int16_t abs_val = abs(audio_buffer[i]);
        if (abs_val > peak) peak = abs_val;
    }

    Serial.printf("[MAIN] Capture done — Peak amplitude: %d / 32767", peak);
    if (peak < 100)       Serial.println(" — Very quiet, check mic wiring");
    else if (peak < 1000) Serial.println(" — Low signal");
    else                  Serial.println(" — Good signal");

    // Send periodic heartbeat via LoRa
    if (millis() - lastHeartbeatTime >= HEARTBEAT_INTERVAL_MS) {
        lora_send_heartbeat();
        lastHeartbeatTime = millis();
    }

    delay(2000);  // Wait before next capture
}
