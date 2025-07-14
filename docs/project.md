# Project Overview: LoRa Gateway Sniffer

## Purpose

This project's primary goal is to develop a mobile LoRa gateway sniffer. The device will be used while traveling to detect LoRaWAN gateways and report both the sniffer's current GPS location and the detected gateway's inferred location to a central LoRaWAN Network Server (LNS).

## Device

The project utilizes the **Heltec Wireless Tracker v1.1** due to its integrated LoRa, GPS, and Wi-Fi capabilities, making it suitable for a mobile sniffing application.

## LoRaWAN Network Server (LNS)

The chosen LNS is a **Chirpstack instance running on the Helium Network**. This provides a decentralized and broad LoRaWAN coverage for data transmission.

## Frequency Configuration

The device will operate on the **US915 sub-band 2 frequency** to align with the Helium network's regional plan for the United States.

## Data Transmission

Upon detecting a gateway, the device will send a LoRaWAN uplink message containing:
1.  The sniffer device's current GPS coordinates.
2.  Information about the detected gateway (e.g., RSSI, SNR, and potentially inferred location based on GPS if available, or just a timestamp and signal strength).

## Security

LoRaWAN security keys (DevEUI, AppEUI, AppKey) are securely stored in `include/secrets.h`. An example file, `include/secrets.h.example`, is provided for reference with placeholder values.

## Expected Workflow

1.  **Initialization:** The device powers on, initializes GPS, LoRa, and connects to Wi-Fi (if configured for auxiliary tasks).
2.  **GPS Acquisition:** Continuously attempts to acquire a stable GPS fix.
3.  **LoRa Scanning:** Actively listens for LoRaWAN gateway pings or uplink transmissions within its configured frequency sub-band.
4.  **Data Packaging:** Upon detecting a gateway signal, packages the sniffer's GPS data, detected gateway signal parameters, and potentially other relevant information into a LoRaWAN payload.
5.  **Uplink Transmission:** Transmits the packaged data as an uplink message to the configured Chirpstack LNS via the Helium network.
6.  **Loop:** Continues scanning and transmitting. 