# Jungle Protection System

A single-node jungle monitoring system for LoRa-based threat detection. Detects illegal chainsaw and mining activity in protected forest areas using edge ML audio classification.

## System Overview

```
┌───────────────────┐         433MHz LoRa         ┌───────────────────┐
│  Forest Sentinel  │ ─────────────────────────▶  │  LoRa Gateway     │
│  (ESP32-S3)       │                             │  (Receiver Node)  │
│                   │                             └────────┬──────────┘
│  • INMP441 Mic    │                                      │ USB Serial
│  • Edge Impulse   │                             ┌────────▼──────────┐
│  • LoRa SX1278    │                             │  Desktop App      │
│  • Solar Powered  │                             │  (Electron)       │
└───────────────────┘                             │  • Node Status    │
                                                  │  • Alert Log      │
                                                  │  • RSSI Chart     │
                                                  │  • Map View       │
                                                  └───────────────────┘
```

## Project Structure

| Directory | Description |
|-----------|-------------|
| [`firmware/`](firmware/) | ESP32-S3 sentinel node firmware (PlatformIO/Arduino) |
| [`desktop-app/`](desktop-app/) | Electron desktop dashboard for monitoring |

## Components

### Firmware (Sentinel Node)
- **Platform:** ESP32-S3 DevKitC-1 N16R8
- **Audio:** INMP441 MEMS I2S microphone @ 16kHz
- **ML:** Edge Impulse on-device inference (chainsaw / mining / ambient)
- **Radio:** LoRa SX1278 433MHz for long-range alert transmission
- **Power:** Solar + 18650 battery with light sleep optimization
- **Alert Logic:** 3 consecutive detections at ≥85% confidence required

See [firmware/README.md](firmware/README.md) for build instructions and wiring guide.

### Desktop App (Dashboard)
- **Framework:** Electron 29
- **Serial:** USB LoRa gateway connection @ 115200 baud
- **Features:** Real-time node status, RSSI chart, alert log with filtering, Leaflet map view
- **Protocol:** Newline-delimited JSON packets via serial port

## Quick Start

### Firmware
```bash
cd firmware
pio run --target upload
pio device monitor --baud 115200
```

### Desktop App
```bash
cd desktop-app
npm install
npm start
```

## Hardware

| Component | Model | Interface |
|-----------|-------|-----------|
| MCU | ESP32-S3 DevKitC-1 N16R8 | — |
| Microphone | INMP441 MEMS | I2S (GPIO 1, 2, 42) |
| LoRa Radio | Ra-02 SX1278 433MHz | SPI (GPIO 18, 19, 23, 5, 14, 26) |
| Power | 18650 + Solar Panel | 3.3V rail |

## Team

B22 Group 03