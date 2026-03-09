# SilentScout - Acoustic Threat Detection System

We're building a system that listens for illegal chainsaw and mining sounds in forests and sends out LoRa alerts when it picks something up. The whole thing runs offline - no WiFi, no cloud - just an ESP32-S3 doing edge inference on audio and pushing alerts over 433MHz LoRa.

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

- **`firmware/`** - ESP32-S3 PlatformIO project (the sensor node)
- **`desktop-app/`** - Electron app for monitoring alerts and node status

## Firmware (sensor node)

- ESP32-S3 DevKitC-1 with 16MB flash and 8MB PSRAM
- INMP441 MEMS mic over I2S, sampling at 16kHz
- Edge Impulse model runs locally - binary classification: `alert` vs `not_alert`
- LoRa SX1278 433MHz for sending alerts (no WiFi needed)
- Solar + 18650 battery, uses light sleep to save power
- Needs 3 consecutive detections at 85%+ confidence before triggering alert

Check [firmware/README.md](firmware/README.md) for wiring and build steps.

## Desktop app

- Electron 29 with serial port connection to gateway
- Shows node online/offline status, RSSI over time, alert log, and a Leaflet map
- Expects newline-delimited JSON at 115200 baud

## Hardware used

| Part | Model | Connection |
|------|-------|------------|
| MCU | ESP32-S3 DevKitC-1 N16R8 | - |
| Mic | INMP441 MEMS | I2S - GPIO 1, 2, 42 |
| Radio | Ra-02 SX1278 433MHz | SPI - GPIO 18, 19, 23, 5, 14, 26 |
| Power | 18650 + Solar Panel | 3.3V rail |

## Dataset

Training data sourced from the [FSC22 dataset on Kaggle](https://www.kaggle.com/datasets/irmiot22/fsc22-dataset/data) - a forest sound classification dataset with 27 audio classes.

We ended up going with a binary classification approach - `alert` vs `not_alert`. Trying to distinguish chainsaw from mining from ambient added complexity without improving the core goal, which is just knowing whether something threatening is happening.

**alert** - any sound associated with illegal logging or mining activity

| Kaggle Class | Reason |
|---|---|
| Chainsaw (#11) | Primary threat signal |
| Axe (#10) | Illegal logging |
| Handsaw (#13) | Manual cutting |
| WoodChop (#16) | Illegal activity |
| Generator (#12) | Powers illegal mining equipment |

**not_alert** - everything else (nature sounds + interference)

| Kaggle Class |
|---|
| Rain (#2), Wind (#5), Silence (#6) |
| BirdChirping (#23), Insect (#21), Frog (#22), WingFlaping (#24) |
| TreeFalling (#7), Squirrel (#27) |
| VehicleEngine (#9), Helicopter (#8), Thunderstorm (#3) |
| WaterDrops (#4), Footsteps (#19), Speaking (#18) |

Collapsing 3+ classes into binary made the model cleaner and the confidence scores more meaningful - a high `alert` score is a strong signal, not just a relative winner between similar classes.

## Model

### Train / Test Split

![Train/Test Split](assets/ss-1.png)

### Accuracy

![Model Accuracy](assets/ss-5.png)

![Confusion Matrix](assets/ss-6.png)

## Design decisions

- Edge inference on the device itself means zero latency waiting on a server and works in dead zones with no connectivity
- The 3-consecutive-readings rule before alerting cuts down on false positives from short sounds like a single axe swing or a car passing by
- LoRa 433MHz was chosen over higher frequencies for better range and foliage penetration in forested terrain
- Solar + light sleep keeps the node alive indefinitely without manual battery swaps, important for remote deployments
- Newline-delimited JSON over serial is deliberately simple - easy to parse, easy to debug with a plain terminal