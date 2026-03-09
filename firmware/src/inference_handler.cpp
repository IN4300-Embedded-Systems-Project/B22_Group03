/* =============================================================================
 * inference_handler.cpp - Edge Impulse Inference Implementation
 *
 * Feeds a captured audio buffer into the Edge Impulse pipeline:
 *   raw PCM -> MFCC DSP block -> TFLite EON model -> classification result
 *
 * The model was trained on the FSC22 dataset, remapped to two labels:
 *   "alert"     - chainsaw or mining activity detected
 *   "not_alert" - ambient or other/interference sounds
 * ========================================================================== */

#include "inference_handler.h"
#include "config.h"

#include <Arduino.h>
#include <Silent_Scout_-_Forest_Protection_inferencing.h>

// ---------------------------------------------------------------------------
// Internal signal wrapper required by Edge Impulse get_data callback
// ---------------------------------------------------------------------------
static const int16_t* _audio_buffer_ptr = nullptr;
static size_t         _audio_buffer_len = 0;

static int ei_get_data(size_t offset, size_t length, float* out_ptr) {
    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = (float)_audio_buffer_ptr[offset + i] / 32768.0f;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Initialize inference engine - validates model parameters against config
// ---------------------------------------------------------------------------
bool inference_init(void) {
    // Confirm the model expects the same sample rate we are capturing at
    if (EI_CLASSIFIER_FREQUENCY != SAMPLE_RATE) {
        Serial.printf("[INFER] ERROR: Model frequency mismatch - model expects %d Hz, config is %d Hz\n",
                      (int)EI_CLASSIFIER_FREQUENCY, SAMPLE_RATE);
        return false;
    }

    Serial.printf("[INFER] Ready - %d samples @ %d Hz, %d labels\n",
                  EI_CLASSIFIER_RAW_SAMPLE_COUNT,
                  (int)EI_CLASSIFIER_FREQUENCY,
                  EI_CLASSIFIER_LABEL_COUNT);
    return true;
}

// ---------------------------------------------------------------------------
// Run inference on a captured audio buffer
// ---------------------------------------------------------------------------
inference_result_t inference_run(const int16_t* buffer, size_t n_samples) {
    inference_result_t result = { .label = "not_alert", .confidence = 0.0f, .valid = false };

    if (buffer == nullptr || n_samples == 0) {
        Serial.println("[INFER] ERROR: Null or empty buffer");
        return result;
    }

    if ((int)n_samples < EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
        Serial.printf("[INFER] ERROR: Buffer too small - got %u, need %d\n",
                      n_samples, EI_CLASSIFIER_RAW_SAMPLE_COUNT);
        return result;
    }

    // Point the get_data callback at the caller's buffer
    _audio_buffer_ptr = buffer;
    _audio_buffer_len = n_samples;

    // Build Edge Impulse signal struct
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data     = &ei_get_data;

    // Run classifier
    ei_impulse_result_t ei_result = { 0 };
    EI_IMPULSE_ERROR err = run_classifier(&signal, &ei_result, false);

    if (err != EI_IMPULSE_OK) {
        Serial.printf("[INFER] ERROR: run_classifier failed (err=%d)\n", (int)err);
        return result;
    }

    // Find the label with the highest confidence
    int   best_idx   = 0;
    float best_score = 0.0f;
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (ei_result.classification[i].value > best_score) {
            best_score = ei_result.classification[i].value;
            best_idx   = i;
        }
    }

    result.label      = ei_result.classification[best_idx].label;
    result.confidence = best_score;
    result.valid      = true;

    Serial.printf("[INFER] Result: %s (%.2f%%) - DSP: %d ms, Classify: %d ms\n",
                  result.label, result.confidence * 100.0f,
                  ei_result.timing.dsp, ei_result.timing.classification);

    return result;
}
