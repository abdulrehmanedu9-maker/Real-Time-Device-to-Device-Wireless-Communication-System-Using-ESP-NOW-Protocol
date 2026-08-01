# Real-Time Device-to-Device Wireless Communication System Using ESP-NOW Protocol

**Phase One — Dual-Sensor Fusion with Confirmation and Cooldown Logic**

A router-free, internet-free, two-node embedded wireless system built on two ESP8266 NodeMCU boards. The Sender node fuses readings from an HC-SR04 ultrasonic sensor and an SR505 PIR motion sensor, applies a stability confirmation and cooldown filter, and transmits a confirmed detection event directly to a Receiver node using the ESP-NOW protocol, with no Wi-Fi router, internet connection, or cloud service involved at any point.

## Overview

Single-sensor detection systems are prone to false triggers, a PIR sensor can react to heat or air movement, and an ultrasonic sensor can react to a stationary object with no real activity behind it. This project addresses that problem by requiring agreement between two independent sensing modalities before any wireless message is sent, and by keeping the entire communication path local through ESP-NOW rather than a conventional Wi-Fi or cloud-based IoT pipeline.

## Key Features

- Dual-sensor fusion: an event is only considered valid when the ultrasonic sensor and the PIR sensor detect something simultaneously.
- One-second confirmation window: the detected distance must remain stable before a packet is transmitted, filtering out brief or accidental triggers.
- Three-second cooldown: prevents repeated transmissions while the same event is still ongoing.
- Fully router-free and internet-free communication using the ESP-NOW protocol at the Wi-Fi data link layer.
- Silent-by-default Receiver that only activates its red LED and prints output when a confirmed detection actually arrives.
- Automatic on-device logging: every confirmed detection is appended to a persistent log file on the Receiver using LittleFS, with no external storage or server required.
- Automatic link-loss handling: the Receiver reverts to a safe state if no packet arrives within a defined timeout.

## Hardware Components

| Component | Quantity | Role |
|---|---|---|
| NodeMCU ESP8266 | 2 | Sender and Receiver microcontrollers |
| HC-SR04 Ultrasonic Sensor | 1 | Distance measurement on Sender |
| SR505 PIR Motion Sensor | 1 | Motion detection on Sender |
| LED (Red, Yellow, Green) | 3 | Sender local status indication |
| LED (Red, Green) | 2 | Receiver status indication |
| 220 ohm Resistors | 5 | Current limiting for LEDs |
| Breadboard and Jumper Wires | as needed | Circuit assembly |
| Micro USB Cable | 2 | Power and programming |

## System Architecture

```
HC-SR04 Ultrasonic ---\
                        --> Sender (ESP8266) --> ESP-NOW --> Receiver (ESP8266) --> LEDs + LittleFS Log
SR505 PIR Motion  -----/
```

The Sender continuously reads both sensors. When both agree on a detection, it starts a one-second confirmation timer against a recorded baseline distance. If the reading stays stable for the full second, a packet is transmitted and a three-second cooldown begins. The Receiver listens continuously, updates its LEDs on packet arrival, prints and logs the event, and falls back to a safe green state after a timeout with no new packets.

## Repository Structure

```
Project
│
├── Demo/
│   └── Live demo.mp4
│
├── Documents/
│   └── Real Time Device to Device Wireless Communication System Using ESP NOW Protocol.pptx
│   └── Real Time Device to Device Wireless Communication System Using ESP NOW Protocol.pdf
│
├── receiver/
│   └── receiver.ino
|
├── Sender/
│   └── Sender.ino
└── README.md
```


## Getting Started

1. Install the Arduino IDE with ESP8266 board support added through the Boards Manager.
2. Wire the Sender board according to the pin mapping documented in `sender.ino` (D1, D2, D3 for LEDs, D5 and D6 for the ultrasonic sensor, D7 for the PIR sensor).
3. Wire the Receiver board according to the pin mapping documented in `receiver.ino` (D1 and D2 for the status LEDs).
4. Update the MAC address constants in both sketches to match your own boards.
5. Upload `sender.ino` to the Sender board and `receiver.ino` to the Receiver board.
6. Open the Serial Monitor on both boards at 115200 baud to observe detection, confirmation, and transmission activity.
7. To retrieve the stored log file, temporarily upload `read_log.ino` to the Receiver board, following the erase-flash precaution noted inside that file, then restore `receiver.ino` afterward.

## Test Summary

The system was validated on hardware across seven test conditions, covering no detection, single-sensor-only triggers, cancelled confirmations, successful confirmed transmissions, sustained detection, and Sender link loss. Results confirmed that single-sensor triggers never reach the Receiver, that brief or unstable detections are correctly filtered out before transmission, and that the Receiver's logged output always matches its Serial Monitor output. Full test cases and captured Serial Monitor output are documented in the project report.

## Limitations

ESP-NOW range on the ESP8266 is limited by the physical environment. The current design supports a single Sender and a single Receiver, and the detection threshold, confirmation window, and cooldown duration are fixed constants requiring a firmware re-upload to change. Log timestamps are based on device uptime rather than real calendar time.

## Future Work

Planned extensions include multi-Sender support with device identification, real-time clock or NTP-based timestamps, a buzzer or mobile notification gateway, an OLED display for live status, runtime-configurable thresholds through a local web interface, and low-power deployment using battery and solar power with ESP8266 deep sleep.

## Author

Abdul Rehman
BSCS, Allama Iqbal Open University

## Supervisor 

Israr Akhter
Slovak Technical University Bratislava
