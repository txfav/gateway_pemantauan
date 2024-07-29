# ESP32 Sensor and Control System with Firebase Integration

This project involves the use of an ESP32 microcontroller to read various sensors, control outputs, and communicate using ESP-NOW. The system also integrates with Firebase for storing and retrieving data, and it includes a web interface for user authentication and configuration.

## Features

- *Sensor Reading*: Reads data from an SRF05 ultrasonic sensor and other environmental sensors.
- *ESP-NOW Communication*: Communicates with nodes and a gateway using the ESP-NOW protocol.
- *Firebase Integration*: Stores sensor data and retrieves configuration data from Firebase.
- *Deep Sleep Mode*: Enters deep sleep mode to save power under certain conditions.
- *Web Interface*: Provides a web interface for user authentication and configuration.
- *Real-time Clock*: Syncs time using NTP for timestamping data.

## Components Used

- ESP32
- SRF05 Ultrasonic Sensor
- LEDs and Buzzer
- EEPROM for storing credentials

## Installation

1. *Clone the repository*
    bash
    git clone https://github.com/yourusername/yourrepository.git
    

2. *Open the project in Arduino IDE or PlatformIO*

3. *Install required libraries*:
    - WiFi
    - esp_wifi
    - esp_now
    - freertos/FreeRTOS.h
    - freertos/queue.h
    - time
    - Firebase ESP Client
    - WebServer
    - EEPROM
    - SRF05

4. *Upload the code* to your ESP32.

## Configuration

### WiFi Settings

Update the WiFi credentials in the code:
```cpp
#define WIFI_SSID "Your_SSID"
#define WIFI_PASSWORD "Your_PASSWORD"
