# ESP32 Sensor and Control System with Firebase Integration

This project involves the use of an ESP32 microcontroller to read various sensors, control outputs, and communicate using ESP-NOW. The system also integrates with Firebase for storing and retrieving data, and it includes a web interface for user authentication and configuration.

## Features

- *Sensor Reading*: Reads data from an SRF05 ultrasonic sensor and other environmental sensors.
- *ESP-NOW Communication*: Communicates with nodes and a gateway using the ESP-NOW protocol.
- *Firebase Integration*: Stores sensor data and retrieves configuration data from Firebase.
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
    git clone [https://github.com/yourusername/yourrepository.git](https://github.com/txfav/gateway_pemantauan.git)
    

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
```
### Firebase Settings

Update the Firebase API key and database URL in the code:
```cpp
#define API_KEY "Your_Firebase_API_Key"
#define DATABASE_URL "Your_Firebase_Database_URL"
```

### Usage
Web Interface use WebServer ESP32
1. AP mode for WebServer
2. Can acces WebInterface via ESP32's IP
3. Log In Firebase

### Sensor Read and Processing Data
- *System Procces Any Data from Node
- *Send and Save Data to Firebase

### Contact
For any inquiries or issues, please contact [faisal.aviva12@gmail.com](mailto:faisal.aviva12@gmail.com).
