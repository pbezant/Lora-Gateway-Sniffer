# LoRa Gateway Sniffer

This project aims to create a LoRa gateway sniffer using the Heltec Wireless Tracker v1.1. The device will capture LoRa gateway information, including its own GPS location and the gateway's location, and send this data to a Chirpstack instance on the Helium network. The project will utilize the US915 sub-band 2 frequency.

## Device Information

- **Board:** Heltec Wireless Tracker v1.1
- **Link:** https://heltec.org/project/wireless-tracker/

## LoRaWAN Configuration

- **Network Server:** Chirpstack instance on Helium Network
- **Frequency Sub-band:** US915 Sub-band 2

## Security Keys

Security keys are stored in `include/secrets.h` and an example `include/secrets.h.example` is provided.
