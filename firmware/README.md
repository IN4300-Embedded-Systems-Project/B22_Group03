# Forest Acoustic Sentinel — Firmware

ESP32-S3 edge ML firmware for detecting illegal chainsaw and mining activity in protected forest areas. Runs on-device audio classification using Edge Impulse and transmits alerts via LoRa 433MHz.

## Hardware Requirements

| Component | Specification |
|-----------|--------------|
| MCU | ESP32-S3 DevKitC-1 N16R8 (16MB Flash, 8MB PSRAM) |
| Microphone | INMP441 MEMS I2S Microphone |
| Radio | LoRa Ra-02 SX1278 433MHz (SPI) |
| Power | Solar + 18650 battery, 3.3V rail |

## Pin Wiring

### INMP441 Microphone (I2S)
| INMP441 Pin | ESP32-S3 GPIO |
|-------------|---------------|
| WS (LRCLK) | GPIO 1 |
| SD (DOUT)   | GPIO 2 |
| SCK (BCLK)  | GPIO 42 |
| L/R         | GND (left channel) |
| VDD         | 3.3V |
| GND         | GND |

### LoRa SX1278 (SPI)
| SX1278 Pin | ESP32-S3 GPIO |
|------------|---------------|
| SCK        | GPIO 18 |
| MISO       | GPIO 19 |
| MOSI       | GPIO 23 |
| NSS (CS)   | GPIO 5 |
| RST        | GPIO 14 |
| DIO0       | GPIO 26 |
| VCC        | 3.3V |
| GND        | GND |

## Build & Flash

### Prerequisites
- [PlatformIO CLI](https://platformio.org/install/cli) or VS Code with PlatformIO extension
- USB-C cable for ESP32-S3

### Edge Impulse Library Setup
1. Export your trained model from Edge Impulse as an **Arduino library**
2. Extract the library folder into `firmware/lib/`
   ```
   firmware/lib/project-1_inferencing/
   ```
3. The model must be trained for **microphone/audio** input with classes: `chainsaw`, `mining`, `ambient`

### Build & Upload
```bash
cd firmware

# Build
pio run

# Upload to ESP32-S3
pio run --target upload

# Monitor serial output
pio device monitor --baud 115200
```

## Configuration

All settings are in [`src/config.h`](src/config.h):

| Setting | Default | Description |
|---------|---------|-------------|
| `NODE_ID` | `"N1"` | Unique node identifier |
| `NODE_LAT` / `NODE_LNG` | `6.9270 / 79.8610` | Deployment coordinates (stub) |
| `CONFIDENCE_THRESHOLD` | `0.85` (85%) | Minimum confidence for valid detection |
| `CONSECUTIVE_REQUIRED` | `3` | Same class N times before alert |
| `ALERT_COOLDOWN_MS` | `10000` (10s) | Cooldown between LoRa transmissions |
| `HEARTBEAT_INTERVAL_MS` | `20000` (20s) | Periodic heartbeat interval |
| `LORA_FREQUENCY` | `433 MHz` | LoRa ISM band frequency |
| `LORA_TX_POWER` | `17 dBm` | Transmit power |

## LoRa Packet Format

Comma-separated string format:

**Alert packet:**
```
N1,alert,chainsaw,0.95,3,123456789,6.9270,79.8610
```

**Heartbeat packet:**
```
N1,data,ambient,0.00,0,123456789,6.9270,79.8610
```

| Field | Description |
|-------|-------------|
| nodeId | Node identifier |
| type | `alert` or `data` |
| alertType | Detected class (`chainsaw`, `mining`, `ambient`) |
| confidence | Classifier confidence (0.00–1.00) |
| consecutiveCount | Number of consecutive detections |
| timestampMs | `millis()` since boot |
| lat | Latitude |
| lng | Longitude |

## Architecture

```
┌─────────────────────────────────────────────┐
│              ESP32-S3 Sentinel              │
│                                             │
│  ┌──────────┐    ┌───────────┐    ┌──────┐  │
│  │ INMP441  │───▶│ Edge      │───▶│ Alert│  │
│  │ I2S Mic  │    │ Impulse   │    │ Logic│  │
│  └──────────┘    │ Inference │    └──┬───┘  │
│                  └───────────┘       │      │
│                                      ▼      │
│                               ┌──────────┐  │
│                               │ LoRa TX  │  │
│                               │ SX1278   │  │
│                               └─────┬────┘  │
│                                     │       │
└─────────────────────────────────────┼───────┘
                                      │ 433MHz
                               ┌──────▼────┐
                               │ LoRa RX   │
                               │ Gateway   │
                               └─────┬─────┘
                                     │ USB
                               ┌─────▼─────┐
                               │ Desktop   │
                               │ Dashboard │
                               └───────────┘
```

## Power Management

- WiFi and Bluetooth disabled at boot
- LoRa radio sleeps between transmissions
- Light sleep between inference cycles (~100ms)
- Watchdog timer (30s) prevents firmware hangs

## Code Structure

| File | Purpose |
|------|---------|
| `src/main.cpp` | Setup, inference loop, alert logic |
| `src/config.h` | All pin definitions and constants |
| `src/audio_capture.h/.cpp` | INMP441 I2S microphone interface |
| `src/lora_handler.h/.cpp` | LoRa SX1278 TX and power control |
