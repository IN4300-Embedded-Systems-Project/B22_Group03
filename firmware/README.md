# SilentScout Firmware

This is the ESP32-S3 firmware for SilentScout. It captures audio from an INMP441 mic, runs Edge Impulse inference to detect chainsaw/mining sounds, and sends LoRa alerts when a threat is confirmed.

Everything runs offline — no WiFi, no cloud. Just the mic, the ML model, and LoRa.

## What you need

| Part | Spec |
|------|------|
| MCU | ESP32-S3 DevKitC-1 N16R8 (16MB Flash, 8MB PSRAM) |
| Mic | INMP441 I2S MEMS Microphone |
| Radio | Ra-02 SX1278 433MHz LoRa module |
| Power | Solar panel + 18650 battery on 3.3V rail |

## Wiring

### INMP441 → ESP32-S3
| INMP441 | GPIO |
|---------|------|
| WS | 1 |
| SD | 2 |
| SCK | 42 |
| L/R | GND |
| VDD | 3.3V |
| GND | GND |

### SX1278 → ESP32-S3
| SX1278 | GPIO |
|--------|------|
| SCK | 18 |
| MISO | 19 |
| MOSI | 23 |
| NSS | 5 |
| RST | 14 |
| DIO0 | 26 |
| VCC | 3.3V |
| GND | GND |

## Building

You need PlatformIO (CLI or the VS Code extension).

### Setting up the Edge Impulse model

1. Go to Edge Impulse, train your model, and export as **Arduino library**
2. Drop the exported folder into `firmware/lib/`:
   ```
   firmware/lib/project-1_inferencing/
   ```
3. Make sure the model is trained on mic input with these classes: `chainsaw`, `mining`, `ambient`

### Flash it

```bash
cd firmware
pio run                        # just build
pio run --target upload        # build + flash
pio device monitor --baud 115200  # serial monitor
```

## Config

Everything's in `src/config.h` — change it before flashing:

| What | Default | Notes |
|------|---------|-------|
| `NODE_ID` | `"N1"` | Change this per node |
| `NODE_LAT` / `NODE_LNG` | `6.9270 / 79.8610` | Hardcoded for now (no GPS) |
| `CONFIDENCE_THRESHOLD` | `85%` | Min confidence to count a detection |
| `CONSECUTIVE_REQUIRED` | `3` | Has to detect same thing 3x in a row |
| `ALERT_COOLDOWN_MS` | `10s` | Wait time between LoRa sends |
| `HEARTBEAT_INTERVAL_MS` | `20s` | How often to send "I'm alive" packet |
| `LORA_FREQUENCY` | `433 MHz` | ISM band |
| `LORA_TX_POWER` | `17 dBm` | Can go up to 20 but uses more battery |

## LoRa packet format

We use a simple CSV string over LoRa. Keeps it compact.

Alert:
```
N1,alert,chainsaw,0.95,3,123456789,6.9270,79.8610
```

Heartbeat:
```
N1,data,ambient,0.00,0,123456789,6.9270,79.8610
```

Fields: `nodeId, type, class, confidence, consecutiveCount, timestamp_ms, lat, lng`

The receiver node will parse this and convert to JSON for the desktop app.

## How the detection works

```
  INMP441 mic
      │
      ▼
  I2S capture (16kHz 16-bit)
      │
      ▼
  Edge Impulse inference
      │
      ├── ambient? → do nothing, reset counter
      │
      ├── chainsaw/mining + confidence >= 85%?
      │       │
      │       ▼
      │   same class 3x in a row?
      │       │
      │       YES → send LoRa alert
      │
      ▼
  light sleep → repeat
```

## Power saving stuff

- WiFi + BT turned off at boot (we don't need them)
- LoRa module sleeps except when actually transmitting
- CPU goes into light sleep between inference cycles
- Watchdog timer set to 30s in case something hangs

## Files

| File | What it does |
|------|-------------|
| `src/main.cpp` | Main loop — capture, infer, alert, sleep, repeat |
| `src/config.h` | Pin defs, thresholds, timing constants |
| `src/audio_capture.h/.cpp` | I2S driver setup and buffer capture |
| `src/lora_handler.h/.cpp` | LoRa init, send alert/heartbeat, sleep/wake |
