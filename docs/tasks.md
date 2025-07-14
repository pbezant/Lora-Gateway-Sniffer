# LoRa Gateway Sniffer: Development Tasks

This document outlines the current and upcoming development tasks for the LoRa Gateway Sniffer project. Tasks are categorized and will be updated as progress is made.

## 1. Initial Setup and Configuration

-   [ ] **Create `include/secrets.h`:** Store LoRaWAN DevEUI, AppEUI, and AppKey. (COMPLETED)
-   [ ] **Create `include/secrets.h.example`:** Provide a template for `secrets.h`. (COMPLETED)
-   [ ] **Configure PlatformIO:** Ensure `platformio.ini` is correctly set up for the Heltec Wireless Tracker v1.1, including board, framework, and necessary libraries.
-   [ ] **Initial Project Documentation:** Create `README.md`, `docs/project.md`, `docs/architecture.md`, `docs/technical.md`, and `docs/memory.md`. (COMPLETED)

## 2. Hardware Interface and Basic Functionality

-   [ ] **GPS Module Integration:**
    -   [ ] Initialize the Quectel L76 GPS module.
    -   [ ] Implement NMEA sentence parsing using TinyGPS++.
    -   [ ] Acquire and store GPS coordinates (latitude, longitude, altitude).
    -   [ ] Handle GPS fix acquisition and loss.
-   [ ] **LoRa Transceiver (SX1276) Setup:**
    -   [ ] Initialize the LoRa module with correct pins and frequency band (US915 sub-band 2).
    -   [ ] Configure LoRaWAN parameters (OTAA, channels).

## 3. LoRaWAN Communication and Sniffing

-   [ ] **LoRaWAN Join Procedure:** Implement the OTAA join process using the keys from `secrets.h`.
-   [ ] **Gateway Sniffing Logic:**
    -   [ ] Implement a mechanism to detect incoming LoRaWAN packets (passive sniffing).
    -   [ ] Extract RSSI and SNR from detected packets.
    -   [ ] (Consider) Extracting any other identifiable gateway information from packets, if feasible and ethical.
-   [ ] **Data Payload Construction:**
    -   [ ] Design a compact binary payload format for GPS data, RSSI, and SNR.
    -   [ ] Encode GPS coordinates and signal parameters into the payload.
-   [ ] **Uplink Transmission:**
    -   [ ] Transmit the constructed payload as a LoRaWAN uplink message.
    -   [ ] Implement basic retransmission or error handling for failed uplinks.

## 4. Power Management and Optimization

-   [ ] **Deep Sleep/Low Power Modes:** Implement power-saving modes for the ESP32 and peripherals when not actively transmitting or waiting for GPS fix.
-   [ ] **Battery Level Monitoring:** (Optional) Add functionality to monitor and report battery voltage.

## 5. Testing and Refinement

-   [ ] **Unit Testing:** Develop unit tests for critical components (e.g., GPS parsing, payload encoding).
-   [ ] **Integration Testing:** Test the full loop of GPS acquisition, gateway sniffing, and LoRaWAN transmission.
-   [ ] **Field Testing:** Conduct real-world tests to verify gateway detection and data reporting accuracy.
-   [ ] **Code Refactoring:** Improve code modularity, readability, and adherence to coding standards.

## 6. Future Enhancements (Out of Scope for Initial Version)

-   [ ] **SD Card Logging:** Log data to an SD card for offline analysis.
-   [ ] **Wi-Fi/Bluetooth for Local Data Access:** Provide a local interface to access logs or configuration.
-   [ ] **Web Interface for Configuration:** Implement a basic web interface on the ESP32 for easy configuration.
-   [ ] **Advanced Gateway Location Estimation:** Use multiple gateway RSSI/SNR values to triangulate or improve gateway location inference. 