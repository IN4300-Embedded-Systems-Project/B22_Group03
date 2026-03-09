# SilentScout — Acoustic Threat Detection System

We're building a system that listens for illegal chainsaw and mining sounds in forests and sends out LoRa alerts when it picks something up. The whole thing runs offline — no WiFi, no cloud — just an ESP32-S3 doing edge inference on audio and pushing alerts over 433MHz LoRa.

## How it works

Basically there's a sensor node sitting out in the forest with a mic and an ML model. When it hears something suspicious (chainsaw, mining equipment), it classifies the sound using Edge Impulse, and if it's confident enough (85%+) for 3 consecutive readings, it fires off a LoRa packet to a gateway node. That gateway is plugged into a laptop running our Electron dashboard.

```
  Sensor Node                   LoRa 433MHz              Gateway              Desktop
┌─────────────┐                                    ┌──────────────┐     ┌──────────────┐
│  ESP32-S3   │ ──────────────────────────────────▶│  LoRa RX     │────▶│  Electron    │
│  INMP441    │            wireless                │  (USB serial)│     │  Dashboard   │
│  SX1278 TX  │                                    └──────────────┘     └──────────────┘
│  Solar+Batt │
└─────────────┘
```

## Repo structure

- **`firmware/`** — ESP32-S3 PlatformIO project (the sensor node)
- **`desktop-app/`** — Electron app for monitoring alerts and node status

## Firmware (sensor node)

- ESP32-S3 DevKitC-1 with 16MB flash and 8MB PSRAM
- INMP441 MEMS mic over I2S, sampling at 16kHz
- Edge Impulse model runs locally — classifies chainsaw / mining / ambient
- LoRa SX1278 433MHz for sending alerts (no WiFi needed)
- Solar + 18650 battery, uses light sleep to save power
- Needs 3 consecutive detections at 85%+ confidence before triggering alert

Check [firmware/README.md](firmware/README.md) for wiring and build steps.

## Desktop app

- Electron 29 with serial port connection to gateway
- Shows node online/offline status, RSSI over time, alert log, and a Leaflet map
- Expects newline-delimited JSON at 115200 baud

## Getting started

**Firmware:**
```bash
cd firmware
pio run --target upload
pio device monitor --baud 115200
```

**Desktop app:**
```bash
cd desktop-app
npm install
npm start
```

## Hardware used

| Part | Model | Connection |
|------|-------|------------|
| MCU | ESP32-S3 DevKitC-1 N16R8 | — |
| Mic | INMP441 MEMS | I2S — GPIO 1, 2, 42 |
| Radio | Ra-02 SX1278 433MHz | SPI — GPIO 18, 19, 23, 5, 14, 26 |
| Power | 18650 + Solar Panel | 3.3V rail |

## Team

B22 Group 03