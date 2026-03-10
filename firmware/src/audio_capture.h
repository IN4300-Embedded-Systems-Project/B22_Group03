/* =============================================================================
 * audio_capture.h - INMP441 I2S Microphone Interface
 *
 * Provides initialization, buffer capture, and cleanup functions for the
 * INMP441 MEMS microphone connected via I2S.
 * ========================================================================== */

#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <stdint.h>
#include <stddef.h>

// ---------------------------------------------------------------------------
// Initialize the I2S peripheral for microphone input
// Parameters:
//   sample_rate - desired sample rate in Hz (e.g., 16000)
// Returns:
//   0 on success, non-zero error code on failure
// ---------------------------------------------------------------------------
int audio_init(uint32_t sample_rate);

// ---------------------------------------------------------------------------
// Capture audio samples into a pre-allocated buffer
// Blocks until the buffer is completely filled.
// Parameters:
//   buffer    - pointer to int16_t array (must be pre-allocated)
//   n_samples - number of 16-bit samples to capture
// Returns:
//   0 on success, -1 on error
// ---------------------------------------------------------------------------
int audio_capture_buffer(int16_t* buffer, size_t n_samples);

// ---------------------------------------------------------------------------
// Deinitialize the I2S peripheral and free DMA resources
// ---------------------------------------------------------------------------
void audio_deinit(void);

#endif // AUDIO_CAPTURE_H
