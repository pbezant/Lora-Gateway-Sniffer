# LoRa Gateway Sniffer: Architecture Overview

This document outlines the high-level architecture of the LoRa Gateway Sniffer, focusing on the main components and their interactions.

## 1. Hardware Layer

- **Heltec Wireless Tracker v1.1:** This board serves as the core hardware platform, integrating:
    -   **ESP32 Microcontroller:** Handles overall program logic, Wi-Fi connectivity, and peripheral control.
    -   **SX1276 LoRa Transceiver:** Manages LoRaWAN communication (sending and receiving packets).
    -   **GPS Module (Quectel L76):** Acquires location data for the sniffer device.
    -   **OLED Display:** (Optional, but present on the board) Can be used for debugging or displaying status information.
    -   **Battery Management:** Handles power from an external battery and charging.

## 2. Software Layer

### 2.1. Main Application Logic

-   **Initialization:** Sets up all hardware components (LoRa, GPS, display).
-   **GPS Management:** Periodically attempts to get a GPS fix. Once a fix is obtained, it stores the coordinates.
-   **LoRaWAN Stack (LMIC/LoRaWAN Library):** Manages the LoRaWAN protocol, including:
    -   **Join Procedure:** Handles the OTAA (Over-The-Air Activation) process using DevEUI, AppEUI, and AppKey.
    -   **Packet Handling:** Prepares data payloads for uplink and potentially handles downlink messages (though less critical for a sniffer).
    -   **Frequency Management:** Configures the LoRa module for the US915 sub-band 2.
-   **Gateway Sniffing Logic:** This is the core functionality that actively listens for LoRaWAN gateway transmissions. This might involve:
    -   **Promiscuous Mode:** If supported by the LoRa module, listening to all LoRa packets.
    -   **RSSI/SNR Measurement:** Capturing signal strength and noise ratio from detected packets.
    -   **Time-on-Air Calculation:** Potentially logging packet duration.
-   **Data Payload Construction:** Formats the sniffer's GPS data and detected gateway parameters into a compact LoRaWAN uplink payload.
-   **Uplink Transmission:** Initiates the sending of the constructed payload to the LoRaWAN Network Server.

### 2.2. Libraries/Frameworks

-   **Arduino Framework:** Provides the base environment for programming the ESP32.
-   **MCCI LoRaWAN LMIC Library:** A common and robust library for implementing the LoRaWAN stack on microcontrollers.
-   **TinyGPS++/Other GPS Library:** Used to parse NMEA sentences from the GPS module and extract location data.
-   **Heltec ESP32 Libraries:** Specific libraries for managing the Heltec board's features (e.g., OLED, LoRa).

## 3. Communication Flow

1.  **GPS Module to ESP32:** GPS data (NMEA sentences) is transmitted via UART from the GPS module to the ESP32.
2.  **ESP32 to LoRa Transceiver:** The ESP32 controls the SX1276 LoRa transceiver via SPI for sending and receiving LoRa packets.
3.  **LoRa Transceiver to LoRaWAN Gateway:** Detected LoRaWAN packets are received by the SX1276 and then processed by the ESP32.
4.  **LoRa Transceiver to LoRaWAN Network Server (LNS):** The ESP32, via the LoRaWAN stack and SX1276, sends uplink messages to nearby LoRaWAN gateways (on the Helium Network).
5.  **LoRaWAN Gateway to Chirpstack (Helium Network):** LoRaWAN gateways forward received uplink messages to the Chirpstack LNS instance running on the Helium Network.
6.  **Chirpstack to Application (User Defined):** The Chirpstack LNS processes the uplink messages, and forwards the data to the configured application, where the sniffer's location and gateway information can be viewed.

## 4. Data Flow (Simplified)

```mermaid
graph TD
    A[Heltec Wireless Tracker v1.1] --> B{GPS Module (L76)}
    A --> C{SX1276 LoRa Transceiver}
    B --> D[GPS Data (Lat/Lon)]
    C --> E[Detected LoRaWAN Packet Data (RSSI, SNR)]
    D & E --> F[ESP32 Microcontroller (Payload Construction)]
    F --> G[LoRaWAN Uplink Message]
    G --> H[LoRaWAN Gateway (Helium Network)]
    H --> I[Chirpstack LNS (Helium Network)]
    I --> J[User Application/Dashboard]
``` 