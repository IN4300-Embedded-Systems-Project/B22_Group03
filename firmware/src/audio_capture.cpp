/* =============================================================================
 * audio_capture.cpp — INMP441 I2S Microphone Implementation
 *
 * Configures I2S peripheral on ESP32-S3 to read 32-bit samples from the
 * INMP441 MEMS microphone, then converts to 16-bit PCM for inference.
 *
 * The INMP441 outputs 24-bit data left-justified in a 32-bit frame.
 * We right-shift by 14 bits to get a usable 16-bit value (keeping the
 * upper 18 bits, truncated to 16).
 * ========================================================================== */

#include "audio_capture.h"
#include "config.h"

#include <Arduino.h>
#include "driver/i2s.h"

// ---------------------------------------------------------------------------
// I2S initialization — configures DMA and pin mapping
// ---------------------------------------------------------------------------
int audio_init(uint32_t sample_rate) {
    // I2S configuration for INMP441
    // - Master mode, receive only
    // - 32-bit per sample (INMP441 outputs 24-bit left-justified in 32-bit)
    // - Mono left channel (INMP441 L/R pin tied to GND = left channel)
    i2s_config_t i2s_config = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate          = sample_rate,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = DMA_BUF_COUNT,
        .dma_buf_len          = DMA_BUF_LEN,
        .use_apll             = false,
        .tx_desc_auto_clear   = false,
        .fixed_mclk           = 0,
    };

    // Pin mapping for INMP441
    i2s_pin_config_t pin_config = {
        .bck_io_num   = I2S_SCK_PIN,   // Serial Clock  (BCLK)
        .ws_io_num    = I2S_WS_PIN,    // Word Select   (LRCLK)
        .data_out_num = I2S_PIN_NO_CHANGE,  // Not used (mic is input only)
        .data_in_num  = I2S_SD_PIN,    // Serial Data   (DOUT from mic)
    };

    // Install I2S driver
    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] ERROR: i2s_driver_install failed (err=%d)\n", err);
        return (int)err;
    }

    // Set I2S pin configuration
    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] ERROR: i2s_set_pin failed (err=%d)\n", err);
        i2s_driver_uninstall(I2S_PORT);
        return (int)err;
    }

    // Clear any stale data in DMA buffers
    i2s_zero_dma_buffer(I2S_PORT);

    Serial.printf("[AUDIO] I2S initialized — %d Hz, pins SCK=%d WS=%d SD=%d\n",
                  sample_rate, I2S_SCK_PIN, I2S_WS_PIN, I2S_SD_PIN);
    return 0;
}

// ---------------------------------------------------------------------------
// Capture audio into a 16-bit buffer
// Reads 32-bit I2S frames and converts to 16-bit PCM
// ---------------------------------------------------------------------------
int audio_capture_buffer(int16_t* buffer, size_t n_samples) {
    if (buffer == nullptr || n_samples == 0) {
        Serial.println("[AUDIO] ERROR: Invalid buffer or sample count");
        return -1;
    }

    // Temporary buffer for 32-bit I2S reads
    const size_t chunk_size = 256;
    int32_t raw32[chunk_size];
    size_t bytes_read = 0;
    size_t collected = 0;

    while (collected < n_samples) {
        // Read a chunk of 32-bit samples from I2S DMA
        esp_err_t err = i2s_read(I2S_PORT, raw32, sizeof(raw32), &bytes_read, 100);
        if (err != ESP_OK) {
            Serial.printf("[AUDIO] ERROR: i2s_read failed (err=%d)\n", err);
            return -1;
        }

        // Convert 32-bit I2S data → 16-bit PCM
        // INMP441 outputs 24-bit data left-justified in 32-bit frame
        // Right-shift by 14 to get useful 16-bit range
        int samples_in_chunk = bytes_read / sizeof(int32_t);
        for (int i = 0; i < samples_in_chunk && collected < n_samples; i++) {
            buffer[collected++] = (int16_t)(raw32[i] >> 14);
        }
    }

    return 0;
}

// ---------------------------------------------------------------------------
// Deinitialize I2S — release DMA and peripheral resources
// ---------------------------------------------------------------------------
void audio_deinit(void) {
    i2s_driver_uninstall(I2S_PORT);
    Serial.println("[AUDIO] I2S deinitialized");
}
