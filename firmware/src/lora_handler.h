/* =============================================================================
 * lora_handler.h — LoRa SX1278 Communication Interface
 *
 * Provides initialization, alert transmission, heartbeat sending,
 * cooldown management, and power control for the LoRa radio module.
 * ========================================================================== */

#ifndef LORA_HANDLER_H
#define LORA_HANDLER_H

#include <stdint.h>

// ---------------------------------------------------------------------------
// Initialize LoRa radio with SPI pins and parameters from config.h
// Returns:
//   true on success, false on failure
// ---------------------------------------------------------------------------
bool lora_init(void);

// ---------------------------------------------------------------------------
// Send an alert packet via LoRa
// Packet format: NODE_ID,alert,ALERT_TYPE,CONFIDENCE,COUNT,TIMESTAMP,LAT,LNG
// Parameters:
//   alertType       — detected threat class (e.g., "chainsaw", "mining")
//   confidence      — classifier confidence (0.0 - 1.0)
//   consecutiveCount — number of consecutive detections
//   timestampMs     — millis() timestamp
// Returns:
//   true if packet sent successfully, false otherwise
// ---------------------------------------------------------------------------
bool lora_send_alert(const char* alertType, float confidence,
                     int consecutiveCount, unsigned long timestampMs);

// ---------------------------------------------------------------------------
// Send a heartbeat/data packet via LoRa
// Packet format: NODE_ID,data,ambient,0.00,0,TIMESTAMP,LAT,LNG
// Returns:
//   true if packet sent successfully, false otherwise
// ---------------------------------------------------------------------------
bool lora_send_heartbeat(void);

// ---------------------------------------------------------------------------
// Check if the transmission cooldown period is still active
// Returns:
//   true if cooldown is active (do not transmit), false if ready to TX
// ---------------------------------------------------------------------------
bool lora_is_cooldown_active(void);

// ---------------------------------------------------------------------------
// Put the LoRa radio into sleep mode to save power
// ---------------------------------------------------------------------------
void lora_sleep(void);

// ---------------------------------------------------------------------------
// Wake the LoRa radio from sleep mode
// ---------------------------------------------------------------------------
void lora_wake(void);

#endif // LORA_HANDLER_H
