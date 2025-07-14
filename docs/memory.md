# LoRa Gateway Sniffer: Memory and Decisions

This document serves as a log for significant implementation decisions, encountered edge cases, solved problems, and rejected approaches during the development of the LoRa Gateway Sniffer. It will evolve as the project progresses.

## 1. Implementation Decisions

-   **Initial LoRaWAN Library:** Decided to use the MCCI LoRaWAN LMIC library due to its robustness, compliance, and wide adoption for ESP32-based LoRaWAN projects. (Initial decision based on common practices and board compatibility).
-   **GPS Library:** Opted for TinyGPS++ for NMEA parsing due to its simplicity and effectiveness in extracting crucial GPS data.
-   **Security Keys Storage:** Storing DevEUI, AppEUI, and AppKey in `include/secrets.h` and excluding it from version control is a standard practice to prevent accidental exposure of sensitive information.
-   **US915 Sub-band 2:** This sub-band was chosen to align with the specified Helium network configuration for the US region.

## 2. Edge Cases and Considerations

-   **GPS Signal Loss:** How the device behaves when GPS signal is lost (e.g., continue transmitting last known location, stop transmitting until new fix, use A-GPS if possible).
-   **LoRaWAN Duty Cycle Limitations:** Ensuring compliance with regional duty cycle regulations (e.g., in US915, this means managing transmission frequency and airtime).
-   **Gateway Detection vs. Uplink Confirmation:** Distinguishing between simply detecting a gateway's presence (passive sniffing) and successfully sending an uplink that is received by the LNS.
-   **Power Management:** Optimizing power consumption for extended battery life during mobile operation, especially when GPS is active.
-   **Data Payload Size:** Keeping the payload small to maximize transmissions and minimize airtime, which might require clever encoding of GPS and gateway data.

## 3. Problems Solved (To be populated as development progresses)

-   *(e.g., "Resolved issue with LMIC library initialization on Heltec board by adjusting SPI pins.")*
-   *(e.g., "Fixed GPS parsing errors by increasing UART buffer size.")*

## 4. Approaches Rejected (To be populated as development progresses)

-   **Why didn't we do something differently?**
    -   *(e.g., "Initially considered using ESP-IDF's native LoRa drivers, but opted for LMIC for quicker LoRaWAN compliance and existing community support.")*
    -   *(e.g., "Explored using Wi-Fi triangulation for location when GPS is unavailable, but rejected due to complexity and power overhead for the current scope.")* 