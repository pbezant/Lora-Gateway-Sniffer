# LoRa Gateway Sniffer: Development Tasks

This document outlines the current and upcoming development tasks for the LoRa Gateway Sniffer project. Tasks are categorized and will be updated as progress is made.

## 🚨 MIGRATION TO ESP32-S3FN8 HELTEC WIRELESS TRACKER V1.1 (IMMEDIATE PRIORITY)

- [ ] Update PlatformIO to use `esp32-s3-devkitc-1` board and correct build flags
- [ ] Update all pin mappings to match [heltec_tracker_v11.h](https://raw.githubusercontent.com/ksjh/HTIT-Tracker/refs/heads/main/include/heltec_tracker_v11.h)
- [ ] Use Heltec library and/or RadioLib for LoRa and display
- [ ] Use ST7735 display driver (160x80, INITR_MINI160x80_PLUGIN)
- [ ] Integrate UC6580 GPS (RX/TX/EN per Heltec v1.1)
- [ ] Implement power management for display, GPS, sensors (VEXT, VTFT, VGNSS)
- [ ] Read battery voltage and display/report it
- [ ] Use user button for page switching
- [ ] Use LED for processing indication
- [ ] Port existing app logic (display pages, LoRaWAN uplink, GPS, etc.)
- [ ] Test all peripherals for reliable operation

## 🚨 CRITICAL BUG FIXES (IMMEDIATE PRIORITY)

### 1. LoRa Radio (SX1262) - Fix -1108 Error & Ensure Reliable Uplink
- [ ] Double-check SX1262 pin mapping to match Heltec v1.1/Meshtastic reference:
    - NSS, DIO1, RST, BUSY pins
- [ ] On startup, clear LoRaWAN session/persistence to force new join (DevNonce issue)
- [ ] Add debug printout of all LoRaWAN session params before join/send
- [ ] Ensure correct LoRaWAN region and sub-band (US915, sub-band 2)
- [ ] Add retry logic and error handling for failed uplinks
- [ ] Test with known-good keys and network

### 2. GPS Module - No Data / No Satellites
- [ ] Set GPS RX to GPIO48, TX to GPIO47, EN to GPIO46 (per Heltec v1.1/Meshtastic)
- [ ] Power GPS via MOSFET (EN pin), ensure it's HIGH to enable
- [ ] Print all incoming serial data from GPS to debug
- [ ] Add a test to print NMEA sentences to serial
- [ ] Confirm GPS module is physically connected and powered
- [ ] Test outdoors to ensure satellite visibility

---

## 1. Initial Setup and Configuration

-   [ ] **Create `include/secrets.h`:** Store LoRaWAN DevEUI, AppEUI, and AppKey. (COMPLETED)
-   [ ] **Create `include/secrets.h.example`:** Provide a template for `secrets.h`. (COMPLETED)
-   [ ] **Configure PlatformIO:** Ensure `platformio.ini` is correctly set up for the Heltec Wireless Tracker v1.1, including board, framework, and necessary libraries. (IN PROGRESS)
-   [ ] **Initial Project Documentation:** Create `README.md`, `docs/project.md`, `docs/architecture.md`, `docs/technical.md`, and `docs/memory.md`. (COMPLETED)

## 2. Hardware Interface and Basic Functionality

-   [ ] **GPS Module Integration:**
    -   [ ] Initialize the UC6580 GPS module.
    -   [ ] Implement NMEA sentence parsing using TinyGPS++.
    -   [ ] Acquire and store GPS coordinates (latitude, longitude, altitude).
    -   [ ] Handle GPS fix acquisition and loss.
-   [ ] **LoRa Transceiver (SX1262) Setup:**
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
-   [ ] **Battery Level Monitoring:** Add functionality to monitor and report battery voltage.

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