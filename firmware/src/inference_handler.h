/* =============================================================================
 * inference_handler.h - Edge Impulse Inference Interface
 *
 * Wraps the Edge Impulse run_classifier() call. Takes a captured audio
 * buffer, runs the MFCC + TFLite pipeline, and returns the top label
 * with its confidence score.
 *
 * Labels: "alert", "not_alert"
 * ========================================================================== */

#ifndef INFERENCE_HANDLER_H
#define INFERENCE_HANDLER_H

#include <stdint.h>
#include <stddef.h>

// ---------------------------------------------------------------------------
// Result structure returned after each inference run
// ---------------------------------------------------------------------------
typedef struct {
    const char* label;      // Winning class label ("alert" or "not_alert")
    float       confidence; // Confidence score  (0.0 - 1.0)
    bool        valid;      // false if inference failed
} inference_result_t;

// ---------------------------------------------------------------------------
// Initialize the inference engine
// Called once in setup() before any inference is run.
// Returns:
//   true on success, false on failure
// ---------------------------------------------------------------------------
bool inference_init(void);

// ---------------------------------------------------------------------------
// Run inference on a pre-captured audio buffer
// Parameters:
//   buffer    - 16-bit PCM samples (must be exactly EI_CLASSIFIER_RAW_SAMPLE_COUNT)
//   n_samples - number of samples in buffer
// Returns:
//   inference_result_t with label, confidence, and valid flag
// ---------------------------------------------------------------------------
inference_result_t inference_run(const int16_t* buffer, size_t n_samples);

#endif // INFERENCE_HANDLER_H
