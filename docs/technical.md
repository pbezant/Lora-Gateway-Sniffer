# LoRa Gateway Sniffer: Technical Specifications and Patterns

This document describes the technical specifications, chosen libraries, and established coding patterns for the LoRa Gateway Sniffer project.

## 1. Development Environment

-   **PlatformIO:** The project is built using PlatformIO, an open-source ecosystem for IoT development. This provides a consistent build environment and simplifies dependency management.
    -   **PlatformIO Configuration:** The `platformio.ini` file configures the project for the `heltec_wifi_kit_32` board (which is compatible with the Wireless Tracker v1.1) and the `espressif32` platform with the `arduino` framework.

## 2. LoRaWAN Implementation

-   **MCCI LoRaWAN LMIC Library:** This library is selected for its robust and compliant implementation of the LoRaWAN protocol stack. It handles:
    -   **OTAA (Over-The-Air Activation):** The device will use OTAA for joining the LoRaWAN network, which is more secure as session keys are negotiated with the network server.
    -   **Frequency Management:** Configuration for US915 sub-band 2 will be handled by LMIC, adhering to regional parameters.
    -   **Class A Device:** The sniffer will operate as a Class A device, allowing for bi-directional communication, where each uplink is followed by two short downlink receive windows.

## 3. GPS Module Interface

-   **Quectel L76 GPS Module:** The integrated GPS module on the Heltec Wireless Tracker v1.1 will be interfaced via UART.
-   **TinyGPS++ Library:** This library will be used for parsing NMEA sentences received from the GPS module, extracting latitude, longitude, altitude, speed, and satellite information.
-   **GPS Fix Strategy:** The device will prioritize obtaining a good GPS fix before transmitting data. If a fix is lost, it will attempt to re-acquire it.

## 4. Data Payload Format

-   **Compact Binary Payload:** To maximize the number of transmissions and comply with LoRaWAN fair use policies, data payloads will be kept as small as possible.
-   **Payload Contents:** The uplink payload will primarily consist of:
    -   **Device Latitude (4 bytes):** Encoded as a fixed-point integer or similar compact format.
    -   **Device Longitude (4 bytes):** Encoded as a fixed-point integer.
    -   **GPS Altitude (2 bytes):** If relevant and space permits.
    -   **Detected Gateway RSSI (1 byte):** Signed integer.
    -   **Detected Gateway SNR (1 byte):** Signed integer.
    -   **Timestamp (4 bytes):** Unix timestamp or relative time for when the gateway signal was detected.
-   **Endianness:** All multi-byte values will be transmitted in little-endian format for consistency.

## 5. Security Key Management

-   **`include/secrets.h`:** LoRaWAN security keys (DevEUI, AppEUI, AppKey) are stored in this file. This file is excluded from version control to prevent accidental exposure.
-   **`include/secrets.h.example`:** A template file is provided for users to easily set up their own keys without modifying the version-controlled example.

## 6. Coding Practices

-   **Modularity:** Code will be organized into logical functions and potentially separate files (e.g., `gps_handler.cpp`, `lora_handler.cpp`) to improve readability and maintainability.
-   **Error Handling:** Basic error handling will be implemented for critical operations (e.g., LoRaWAN join failures, GPS acquisition errors) with serial output for debugging.
-   **Low Power Considerations:** Efforts will be made to optimize power consumption, especially when GPS is actively searching for a fix or when the device is idle. This may involve putting components into low-power modes.
-   **Serial Debugging:** Extensive use of `Serial.print()` will be made for development and debugging purposes. This will be conditionally compiled out for release builds. 