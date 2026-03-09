/* =============================================================================
 * power_manager.h - ESP32-S3 Power Management Interface
 *
 * Provides light sleep between inference cycles to reduce average current
 * draw. The node wakes on timer, runs one inference window, then sleeps
 * again until the next cycle.
 * ========================================================================== */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <stdint.h>

// ---------------------------------------------------------------------------
// Initialize power management - configures wake-up source
// Called once in setup().
// ---------------------------------------------------------------------------
void power_init(void);

// ---------------------------------------------------------------------------
// Enter light sleep for the configured duration
// CPU halts but RAM and peripherals are retained. Wakes on timer.
// Defined by LIGHT_SLEEP_DURATION_US in config.h
// ---------------------------------------------------------------------------
void power_light_sleep(void);

// ---------------------------------------------------------------------------
// Read the supply voltage from the internal ADC
// Returns estimated battery voltage in volts, or 0.0 if not supported.
// ---------------------------------------------------------------------------
float power_read_voltage(void);

#endif // POWER_MANAGER_H
