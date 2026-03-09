/* =============================================================================
 * Forest Acoustic Sentinel — Main Firmware
 * ESP32-S3 + INMP441 + LoRa SX1278
 *
 * Detects illegal chainsaw and mining activity using edge ML inference
 * and transmits alerts via LoRa 433MHz.
 * ========================================================================== */

// Disable quantized filterbank for better accuracy on ESP32-S3
#define EIDSP_QUANTIZE_FILTERBANK 0

#include <Arduino.h>
#include <project-1_inferencing.h>
#include "config.h"
#include "audio_capture.h"
#include "lora_handler.h"

// Power management includes
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

// ---------------------------------------------------------------------------
// Audio buffer — allocated in PSRAM, sized to Edge Impulse model input
// ---------------------------------------------------------------------------
static int16_t* audio_buffer = nullptr;

// ---------------------------------------------------------------------------
// Heartbeat tracking
// ---------------------------------------------------------------------------
static unsigned long lastHeartbeatTime = 0;

// ---------------------------------------------------------------------------
// Alert logic state — consecutive detection for false positive reduction
// ---------------------------------------------------------------------------
static char   lastDetectedClass[32] = "";  // Last non-ambient class detected
static int    consecutiveCount      = 0;    // Consecutive same-class detections
static float  lastConfidence        = 0.0f; // Confidence of last valid detection
static bool   alertConfirmed        = false;// True when consecutive threshold met

// ---------------------------------------------------------------------------
// Inference cycle counter for diagnostics
// ---------------------------------------------------------------------------
static unsigned long inferenceCount  = 0;
static unsigned long errorCount      = 0;

// ---------------------------------------------------------------------------
// Edge Impulse callback — converts int16 audio samples to float
// Used by the EI signal_t interface
// ---------------------------------------------------------------------------
static int audio_signal_get_data(size_t offset, size_t length, float* out_ptr) {
    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = (float)audio_buffer[offset + i] / 32768.0f;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial && millis() < 3000);

    // ---- Power: Disable WiFi and Bluetooth radios to save power ----
    esp_wifi_stop();
    esp_bt_controller_disable();
    Serial.println("[POWER] WiFi and Bluetooth disabled");

    // ---- Print boot reason for diagnostics ----
    esp_reset_reason_t reason = esp_reset_reason();
    Serial.printf("[MAIN] Boot reason: %d", reason);
    switch (reason) {
        case ESP_RST_POWERON:  Serial.println(" (power-on)");   break;
        case ESP_RST_SW:       Serial.println(" (software)");   break;
        case ESP_RST_PANIC:    Serial.println(" (panic/crash)"); break;
        case ESP_RST_WDT:      Serial.println(" (watchdog)");   break;
        case ESP_RST_DEEPSLEEP:Serial.println(" (deep sleep)"); break;
        default:               Serial.println(" (other)");      break;
    }

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

    // Print Edge Impulse model info
    Serial.printf("[INFER] Model frame size : %d samples\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    Serial.printf("[INFER] Sample length    : %d ms\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    Serial.printf("[INFER] Interval         : %.2f ms\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    Serial.printf("[INFER] Number of classes: %d\n",
                  sizeof(ei_classifier_inferencing_categories) /
                  sizeof(ei_classifier_inferencing_categories[0]));
    Serial.println();

    // Initialize I2S microphone
    if (audio_init(SAMPLE_RATE) != 0) {
        Serial.println("[MAIN] FATAL: Audio init failed — halting");
        while (true) { delay(1000); }
    }

    // Allocate audio buffer in PSRAM sized to EI model input
    size_t buffer_size = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    audio_buffer = (int16_t*)ps_malloc(buffer_size * sizeof(int16_t));
    if (audio_buffer == nullptr) {
        Serial.printf("[MAIN] FATAL: Failed to allocate audio buffer (%d bytes)\n",
                      buffer_size * sizeof(int16_t));
        while (true) { delay(1000); }
    }
    Serial.printf("[MAIN] Audio buffer allocated: %d samples in PSRAM\n", buffer_size);

    // Initialize LoRa radio
    if (!lora_init()) {
        Serial.println("[MAIN] WARNING: LoRa init failed — continuing without radio");
    } else {
        lora_send_heartbeat();
        lastHeartbeatTime = millis();
    }

    Serial.println("[MAIN] Sentinel active — starting inference loop\n");

    // ---- Initialize watchdog timer (30s timeout) ----
    esp_task_wdt_init(30, true);  // 30 second timeout, panic on trigger
    esp_task_wdt_add(NULL);       // Add current task to WDT
    Serial.println("[MAIN] Watchdog timer initialized (30s)");
}

// ---------------------------------------------------------------------------
// Main Loop — Capture audio → Run inference → Print results
// ---------------------------------------------------------------------------
void loop() {
    // Reset watchdog timer at start of each cycle
    esp_task_wdt_reset();

    inferenceCount++;

    // ---- Step 1: Capture audio ----
    int result = audio_capture_buffer(audio_buffer, EI_CLASSIFIER_RAW_SAMPLE_COUNT);
    if (result != 0) {
        errorCount++;
        Serial.printf("[AUDIO] ERROR: Capture failed (errors: %lu/%lu)\n",
                      errorCount, inferenceCount);
        delay(1000);
        return;
    }

    // ---- Step 2: Run Edge Impulse classifier ----
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data     = &audio_signal_get_data;

    ei_impulse_result_t ei_result = { 0 };
    EI_IMPULSE_ERROR err = run_classifier(&signal, &ei_result, false);
    if (err != EI_IMPULSE_OK) {
        errorCount++;
        Serial.printf("[INFER] ERROR: Classifier failed (err=%d, errors: %lu/%lu)\n",
                      err, errorCount, inferenceCount);
        delay(1000);
        return;
    }

    // ---- Step 3: Print classification results ----
    Serial.printf("[INFER] #%lu | DSP: %d ms | Classification: %d ms\n",
                  inferenceCount, ei_result.timing.dsp, ei_result.timing.classification);

    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        Serial.printf("[INFER]   %s: %.4f\n",
                      ei_result.classification[ix].label,
                      ei_result.classification[ix].value);
    }

    // ---- Step 4: Alert logic — consecutive detection tracking ----
    // Find the class with the highest confidence score
    float  maxConfidence = 0.0f;
    int    maxIndex      = 0;
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        if (ei_result.classification[ix].value > maxConfidence) {
            maxConfidence = ei_result.classification[ix].value;
            maxIndex = ix;
        }
    }

    const char* topClass = ei_result.classification[maxIndex].label;

    // Only count detections above confidence threshold and not "ambient"
    if (maxConfidence >= CONFIDENCE_THRESHOLD && strcmp(topClass, "ambient") != 0) {
        // Check if same class as last detection
        if (strcmp(topClass, lastDetectedClass) == 0) {
            consecutiveCount++;
        } else {
            // Different threat class — reset counter
            consecutiveCount = 1;
            strncpy(lastDetectedClass, topClass, sizeof(lastDetectedClass) - 1);
            lastDetectedClass[sizeof(lastDetectedClass) - 1] = '\0';
        }
        lastConfidence = maxConfidence;

        Serial.printf("[ALERT] Threat: %s (%.1f%%) — consecutive: %d/%d\n",
                      topClass, maxConfidence * 100.0f, consecutiveCount,
                      CONSECUTIVE_REQUIRED);

        // Check if consecutive threshold is met
        if (consecutiveCount >= CONSECUTIVE_REQUIRED) {
            alertConfirmed = true;
            Serial.printf("[ALERT] >>> CONFIRMED: %s detected %d consecutive times! <<<\n",
                          lastDetectedClass, consecutiveCount);
        }
    } else {
        // Ambient or below threshold — reset consecutive counter
        if (consecutiveCount > 0) {
            Serial.printf("[ALERT] Reset — %s (%.1f%%) below threshold or ambient\n",
                          topClass, maxConfidence * 100.0f);
        }
        consecutiveCount = 0;
        lastDetectedClass[0] = '\0';
        alertConfirmed = false;
    }

    // ---- Step 5: LoRa alert transmission on confirmed threat ----
    if (alertConfirmed && !lora_is_cooldown_active()) {
        Serial.printf("[LORA] Sending alert: %s (%.1f%%, %dx)\n",
                      lastDetectedClass, lastConfidence * 100.0f, consecutiveCount);

        bool sent = lora_send_alert(lastDetectedClass, lastConfidence,
                                    consecutiveCount, millis());
        if (sent) {
            Serial.println("[LORA] Alert packet transmitted successfully");
        } else {
            Serial.println("[LORA] ERROR: Failed to send alert packet");
        }

        // Reset alert state after transmission attempt
        alertConfirmed = false;
    } else if (alertConfirmed && lora_is_cooldown_active()) {
        Serial.println("[LORA] Alert confirmed but cooldown active — skipping TX");
    }

    // ---- Step 6: Send periodic heartbeat ----
    if (millis() - lastHeartbeatTime >= HEARTBEAT_INTERVAL_MS) {
        lora_send_heartbeat();
        lastHeartbeatTime = millis();
    }

    // ---- Step 7: Light sleep between inference cycles ----
    // Use timed light sleep to reduce power consumption
    // CPU halts but peripherals stay active, wakes on timer
    Serial.flush();  // Ensure all serial data is sent before sleeping
    esp_sleep_enable_timer_wakeup(LIGHT_SLEEP_DURATION_US);
    esp_light_sleep_start();
}

// ---------------------------------------------------------------------------
// Sensor validation — ensure model was trained for microphone input
// ---------------------------------------------------------------------------
#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor — must be a microphone/audio model."
#endif
