# Real-Time Device-to-Device Wireless Communication System Using ESP-NOW Protocol
**Phase 01**

## Overview

This project presents a real-time wireless communication system developed using two ESP8266 NodeMCU boards and the ESP-NOW protocol. The system enables direct device-to-device communication without requiring a Wi-Fi router, internet connection, or cloud platform.

The sender node combines data from an HC-SR04 ultrasonic sensor and an SR505 PIR motion sensor to detect nearby objects and motion. When both sensors detect an event simultaneously, an alert message is transmitted wirelessly to the receiver node. The receiver then activates an alert LED in real time.

## Features

- Real-time device-to-device communication
- ESP-NOW peer-to-peer wireless protocol
- No Wi-Fi router or internet required
- Dual sensor detection using HC-SR04 and SR505 PIR
- Real-time LED status indication
- Low-cost IoT security monitoring system

## Hardware Used

- ESP8266 NodeMCU (2 Boards)
- HC-SR04 Ultrasonic Sensor
- SR505 PIR Motion Sensor
- LEDs (Red, Yellow, Green)
- 220Ω Resistors
- Breadboards
- Jumper Wires
- USB Cables

## Software Used

- Arduino IDE
- ESP8266 Board Package
- ESP-NOW Library
- Git & GitHub

## Project Structure

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

## Working Principle

- The sender continuously reads the ultrasonic and PIR sensors.
- If both sensors detect an event simultaneously, a detection packet is sent using ESP-NOW.
- The receiver receives the packet and turns ON the Red LED.
- If no packet is received, the receiver remains in the safe state by turning ON the Green LED.

## Applications

- Home Security
- Restricted Area Monitoring
- Smart Entry Detection
- Wireless Sensor Networks
- IoT Education Projects

## Supervisor

**Israr Akhter**  
Slovak University of Technology in Bratislava

## Author

**Abdul Rehman**  
Allama Iqbal Open Uniuversity (AIOU)

## License

This project is developed for educational and academic purposes.
