/* =============================================================================
 * power_manager.cpp - ESP32-S3 Power Management Implementation
 *
 * Uses esp_sleep_enable_timer_wakeup() + esp_light_sleep_start() to put
 * the ESP32-S3 into light sleep between inference cycles. Light sleep keeps
 * RAM alive so no re-initialization is needed on wake-up.
 *
 * Battery voltage is read from the internal ADC on GPIO 34 via a resistor
 * divider. If your board doesn't have one, power_read_voltage() returns 0.
 * ========================================================================== */

#include "power_manager.h"
#include "config.h"

#include <Arduino.h>
#include "esp_sleep.h"

// ---------------------------------------------------------------------------
// Initialize power management - set timer as the wake-up source
// ---------------------------------------------------------------------------
void power_init(void) {
    // Configure timer wake-up source once - duration set per sleep call
    esp_sleep_enable_timer_wakeup(LIGHT_SLEEP_DURATION_US);

    Serial.printf("[POWER] Light sleep configured - wake interval %llu ms\n",
                  LIGHT_SLEEP_DURATION_US / 1000ULL);
}

// ---------------------------------------------------------------------------
// Enter light sleep for LIGHT_SLEEP_DURATION_US microseconds
// Returns immediately after the CPU wakes from timer
// ---------------------------------------------------------------------------
void power_light_sleep(void) {
    // Refresh the timer duration before each sleep
    esp_sleep_enable_timer_wakeup(LIGHT_SLEEP_DURATION_US);
    esp_light_sleep_start();
    // Execution continues here after wake-up
}

// ---------------------------------------------------------------------------
// Read battery voltage from ADC (GPIO 34, 1/2 voltage divider assumed)
// Returns 0.0 if the pin is not connected or no divider is present
// ---------------------------------------------------------------------------
float power_read_voltage(void) {
    // ADC1 channel on GPIO 34 - assumes a 100k/100k voltage divider
    // Vbat = Vadc * 2  (divider ratio 0.5)
    // ADC reference is 3.3V, 12-bit resolution (0-4095)
    int raw = analogRead(34);
    if (raw == 0) {
        return 0.0f;
    }
    float vadc = (raw / 4095.0f) * 3.3f;
    float vbat = vadc * 2.0f;
    return vbat;
}
