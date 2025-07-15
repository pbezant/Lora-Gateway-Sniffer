#include "lora_handler.h"
#include "secrets.h"
#include <SPI.h>
#include "Config.h"

// Define static constants
const float LoRaHandler::SIGNAL_CHANGE_THRESHOLD = 10.0; // dBm change threshold
const unsigned long LoRaHandler::MIN_DISCOVERY_INTERVAL = 30000; // 30 seconds minimum between discoveries

// Add global or class member for SPI
SPIClass spiLoRa(FSPI);

LoRaHandler::LoRaHandler() : 
    radio(nullptr), 
    node(nullptr), 
    initialized(false), 
    joined(false), 
    lastSendTime(0), 
    lastJoinAttempt(0),
    lastErrorCode(0),
    lastRssi(0.0),
    lastSnr(0.0),
    gatewayDiscoveryEnabled(true),
    lastGatewayRssi(-999.0),
    lastGatewaySnr(-999.0),
    lastGatewayDiscoveryTime(0) {
    Serial.println(F("[LoRa] Handler created"));
}

LoRaHandler::~LoRaHandler() {
    if (node) {
        delete node;
        node = nullptr;
    }
    if (radio) {
        delete radio;
        radio = nullptr;
    }
    Serial.println(F("[LoRa] Handler destroyed"));
}

bool LoRaHandler::initialize() {
    Serial.println(F("[LoRa] Initializing LoRa handler..."));
    Serial.printf("[LoRa] Pin mapping: CS=%d, DIO1=%d, RST=%d, BUSY=%d, SCK=%d, MISO=%d, MOSI=%d\n", LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY, LORA_SCK, LORA_MISO, LORA_MOSI);

    // Ensure VEXT is enabled before LoRa init (if not already done)
    pinMode(VEXT_PIN, OUTPUT);
    digitalWrite(VEXT_PIN, HIGH);
    Serial.println(F("[LoRa] VEXT power enabled for LoRa"));
    delay(50); // Let power rail stabilize

    // Initialize FSPI for LoRa
    spiLoRa.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    Serial.println(F("[LoRa] FSPI bus initialized for LoRa"));

    // Create radio instance with explicit SPI (pass reference, not pointer)
    radio = new SX1262(new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY, spiLoRa));
    if (!radio) {
        Serial.println(F("[LoRa] [ERROR] Failed to create radio instance"));
        return false;
    }
    
    // Initialize radio hardware
    Serial.println(F("[LoRa] Initializing radio hardware..."));
    int16_t state = radio->begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] [ERROR] Radio initialization failed, code: %d\n", state);
        lastErrorCode = state;
        return false;
    }
    
    // Create LoRaWAN node with US915 band and subband 2
    node = new LoRaWANNode(radio, &US915, 2);
    if (!node) {
        Serial.println(F("[LoRa] [ERROR] Failed to create LoRaWAN node"));
        return false;
    }
    
    Serial.println(F("[LoRa] [SUCCESS] Radio hardware initialized"));
    initialized = true;
    return true;
}

bool LoRaHandler::configureCredentials() {
    if (!initialized) {
        Serial.println(F("[LoRa] [ERROR] Handler not initialized"));
        return false;
    }
    
    Serial.println(F("[LoRa] Configuring credentials..."));
    
    // Print credentials for verification
    printCredentials();
    
    // Convert credentials to 64-bit integers (MSB format)
    uint64_t joinEUI = 0;
    uint64_t devEUI = 0;
    
    for (int i = 0; i < 8; i++) {
        joinEUI = (joinEUI << 8) | APPEUI[i];
        devEUI = (devEUI << 8) | DEVEUI[i];
    }
    
    // Configure credentials
    node->beginOTAA(joinEUI, devEUI, (uint8_t*)APPKEY, (uint8_t*)APPKEY);
    
    Serial.println(F("[LoRa] [SUCCESS] Credentials configured"));
    return true;
}

bool LoRaHandler::joinNetwork() {
    if (!initialized) {
        Serial.println(F("[LoRa] [ERROR] Handler not initialized"));
        return false;
    }
    
    Serial.println(F("[LoRa] =========================================="));
    Serial.println(F("[LoRa] Attempting OTAA join..."));
    Serial.println(F("[LoRa] =========================================="));
    
    Serial.println(F("[LoRa] [DEBUG] Sending join request..."));
    unsigned long joinStartTime = millis();
    
    // Attempt to join with a reasonable timeout
    int16_t state = node->activateOTAA();
    
    if (state == RADIOLIB_ERR_NONE) {
        unsigned long joinTime = millis() - joinStartTime;
        Serial.printf("[LoRa] [SUCCESS] ✅ Join completed in %lu ms\n", joinTime);
        
        // Verify session was established
        if (node->isActivated()) {
            Serial.println(F("[LoRa] [SUCCESS] ✅ Session established"));
            
            // Update signal quality
            lastRssi = radio->getRSSI();
            lastSnr = radio->getSNR();
            
            joined = true;
            printJoinStatus();
            
            // Remove initial test message - no longer needed
            // delay(1000); // Give network time to process
            // String testMessage = "Online!";
            // if (sendData(testMessage)) {
            //     Serial.println(F("[LoRa] [SUCCESS] ✅ Initial test message sent"));
            // } else {
            //     Serial.println(F("[LoRa] [WARN] Failed to send initial test message"));
            // }
            
            return true;
        } else {
            Serial.println(F("[LoRa] [ERROR] ❌ Join reported success but session not established"));
            joined = false;
            return false;
        }
    } else {
        unsigned long joinTime = millis() - joinStartTime;
        Serial.printf("[LoRa] [ERROR] ❌ Join failed after %lu ms\n", joinTime);
        
        lastErrorCode = state;
        Serial.printf("[LoRa] [ERROR] Join failed with error code: %d (%s)\n", state, getErrorString(state).c_str());
        
        Serial.println(F("[LoRa] [ERROR] =========================================="));
        Serial.println(F("[LoRa] [ERROR] Join failure analysis:"));
        Serial.println(F("[LoRa] [ERROR] - Device is reaching ChirpStack (check logs)"));
        Serial.println(F("[LoRa] [ERROR] - Problem is likely in join accept handling"));
        Serial.println(F("[LoRa] [ERROR] - Try clearing persistence and rejoining"));
        Serial.println(F("[LoRa] [ERROR] =========================================="));
        
        joined = false;
        return false;
    }
}

bool LoRaHandler::sendData(const String& data, uint8_t port, bool confirmed) {
    if (!initialized || !joined) {
        Serial.println(F("[LoRa] [ERROR] Not initialized or not joined"));
        return false;
    }

    // Print current frame counter (if available)
    Serial.printf("[LoRa] [DEBUG] Before uplink: isActivated=%d\n", node->isActivated());
    Serial.printf("[LoRa] [DEBUG] Uplink frame counter (fCnt): %lu\n", node->getFCntUp());

    Serial.printf("[LoRa] Sending data on port %d: %s\n", port, data.c_str());

    String dataToSend = data; // Create non-const copy
    int16_t state = node->uplink(dataToSend, port, confirmed);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("[LoRa] [SUCCESS] ✅ Data sent successfully"));
        lastSendTime = millis();
        lastRssi = radio->getRSSI();
        lastSnr = radio->getSNR();
        Serial.printf("[LoRa] [DEBUG] After uplink: isActivated=%d\n", node->isActivated());
        Serial.printf("[LoRa] [DEBUG] Uplink frame counter (fCnt): %lu\n", node->getFCntUp());
        return true;
    } else {
        Serial.printf("[LoRa] [ERROR] ❌ Failed to send data, code: %d (%s)\n", state, getErrorString(state).c_str());
        lastErrorCode = state;
        // If -1108, try to clear persistence and rejoin
        if (state == -1108) {
            Serial.println(F("[LoRa] [WARN] -1108 error: clearing persistence and rejoining..."));
            clearPersistence();
            joinNetwork();
        }
        return false;
    }
}

bool LoRaHandler::sendGPSData(float latitude, float longitude, float altitude, int satellites) {
    if (!initialized || !joined) {
        Serial.println(F("[LoRa] [ERROR] Not initialized or not joined"));
        return false;
    }
    
    // Create GPS data payload
    String gpsData = "{\"lat\":" + String(latitude, 6) + 
                     ",\"lon\":" + String(longitude, 6) + 
                     ",\"alt\":" + String(altitude, 1) + 
                     ",\"sats\":" + String(satellites) + "}";
    
    return sendData(gpsData, 2);
}

bool LoRaHandler::sendStatusData(unsigned long uptime, size_t freeHeap, float batteryVoltage, float batteryPercentage, bool hasGPS, float lat, float lon, float alt, int sats) {
    if (!initialized || !joined) {
        Serial.println(F("[LoRa] [ERROR] Not initialized or not joined"));
        return false;
    }
    
    // Create binary payload to minimize size
    uint8_t payload[24];  // Max 24 bytes: 11 for status + 13 for GPS
    uint8_t payloadSize = 0;
    
    // Pack status data into binary format
    // Uptime (4 bytes) - seconds as uint32
    uint32_t uptimeSeconds = uptime / 1000;
    payload[payloadSize++] = (uptimeSeconds >> 24) & 0xFF;
    payload[payloadSize++] = (uptimeSeconds >> 16) & 0xFF;
    payload[payloadSize++] = (uptimeSeconds >> 8) & 0xFF;
    payload[payloadSize++] = uptimeSeconds & 0xFF;
    
    // Free heap (2 bytes) - KB as uint16
    uint16_t heapKB = freeHeap / 1024;
    payload[payloadSize++] = (heapKB >> 8) & 0xFF;
    payload[payloadSize++] = heapKB & 0xFF;
    
    // RSSI (1 byte) - offset by 200 to fit in uint8 (so -100 dBm = 100)
    uint8_t rssiOffset = (uint8_t)((int)lastRssi + 200);
    payload[payloadSize++] = rssiOffset;
    
    // SNR (1 byte) - multiply by 4 for 0.25 precision, offset by 128
    int8_t snrScaled = (int8_t)((lastSnr * 4) + 128);
    payload[payloadSize++] = (uint8_t)snrScaled;
    
    // Battery voltage (2 bytes) - millivolts as uint16
    uint16_t batteryMV = (uint16_t)(batteryVoltage * 1000);
    payload[payloadSize++] = (batteryMV >> 8) & 0xFF;
    payload[payloadSize++] = batteryMV & 0xFF;
    
    // Battery percentage (1 byte)
    payload[payloadSize++] = (uint8_t)batteryPercentage;
    
    // Add GPS data if available
    if (hasGPS) {
        // Latitude (4 bytes) - float32
        union { float f; uint32_t i; } latUnion;
        latUnion.f = lat;
        payload[payloadSize++] = (latUnion.i >> 24) & 0xFF;
        payload[payloadSize++] = (latUnion.i >> 16) & 0xFF;
        payload[payloadSize++] = (latUnion.i >> 8) & 0xFF;
        payload[payloadSize++] = latUnion.i & 0xFF;
        
        // Longitude (4 bytes) - float32
        union { float f; uint32_t i; } lonUnion;
        lonUnion.f = lon;
        payload[payloadSize++] = (lonUnion.i >> 24) & 0xFF;
        payload[payloadSize++] = (lonUnion.i >> 16) & 0xFF;
        payload[payloadSize++] = (lonUnion.i >> 8) & 0xFF;
        payload[payloadSize++] = lonUnion.i & 0xFF;
        
        // Altitude (4 bytes) - float32
        union { float f; uint32_t i; } altUnion;
        altUnion.f = alt;
        payload[payloadSize++] = (altUnion.i >> 24) & 0xFF;
        payload[payloadSize++] = (altUnion.i >> 16) & 0xFF;
        payload[payloadSize++] = (altUnion.i >> 8) & 0xFF;
        payload[payloadSize++] = altUnion.i & 0xFF;
        
        // Satellites (1 byte)
        payload[payloadSize++] = (uint8_t)sats;
    }
    
    Serial.printf("[LoRa] Sending binary payload: %d bytes\n", payloadSize);
    Serial.print("[LoRa] Hex: ");
    for (int i = 0; i < payloadSize; i++) {
        Serial.printf("%02X ", payload[i]);
    }
    Serial.println();
    
    // Send the binary payload using RadioLib
    int result = node->uplink(payload, payloadSize, 3);
    
    if (result == RADIOLIB_ERR_NONE) {
        Serial.println(F("[LoRa] [SUCCESS] Binary data sent successfully"));
        lastRssi = radio->getRSSI();
        lastSnr = radio->getSNR();
        lastErrorCode = RADIOLIB_ERR_NONE;
        return true;
    } else {
        Serial.printf("[LoRa] [ERROR] Failed to send binary data, code: %d (%s)\n", result, getErrorString(result).c_str());
        lastErrorCode = result;
        return false;
    }
}

void LoRaHandler::handlePeriodicTasks() {
    if (!initialized) return;
    
    // Attempt to rejoin if not connected and enough time has passed
    if (!joined && (millis() - lastJoinAttempt > LORA_RETRY_DELAY)) {
        Serial.println(F("[LoRa] [INFO] Attempting periodic rejoin..."));
        joinNetwork();
        lastJoinAttempt = millis();
    }
}

bool LoRaHandler::shouldSendData() const {
    if (!initialized || !joined) return false;
    return (millis() - lastSendTime) > LORA_SEND_INTERVAL;
}

void LoRaHandler::resetDevNonce() {
    if (!initialized || !node) {
        Serial.println(F("[LoRa] [ERROR] Cannot reset DevNonce - not initialized"));
        return;
    }
    
    Serial.println(F("[LoRa] [INFO] Resetting DevNonce..."));
    
    // Generate a new random DevNonce
    uint16_t newDevNonce = random(0x0001, 0xFFFF);
    
    // Reset the LoRaWAN node - this will clear session and force new join
    joined = false;
    
    Serial.printf("[LoRa] [SUCCESS] DevNonce reset. Next join will use new DevNonce: %u (0x%04X)\n", 
                  newDevNonce, newDevNonce);
    Serial.println(F("[LoRa] [INFO] Device will use new DevNonce on next join attempt"));
}

uint16_t LoRaHandler::getCurrentDevNonce() const {
    if (!initialized || !node) {
        Serial.println(F("[LoRa] [ERROR] Cannot get DevNonce - not initialized"));
        return 0;
    }
    
    // RadioLib manages DevNonce internally, we can't directly query it
    // Return a placeholder value
    Serial.println(F("[LoRa] [INFO] DevNonce is managed internally by RadioLib"));
    Serial.println(F("[LoRa] [INFO] Use 'clear_persistence' to force new join with fresh DevNonce"));
    return 0xFFFF; // Placeholder value
}

void LoRaHandler::clearPersistence() {
    if (!initialized || !node) {
        Serial.println(F("[LoRa] [ERROR] Cannot clear persistence - not initialized"));
        return;
    }
    
    Serial.println(F("[LoRa] [DEBUG] Clearing LoRaWAN session persistence..."));
    
    // Reset join state to force fresh OTAA join
    joined = false;
    lastErrorCode = 0;
    
    // Clear any RadioLib persistence (this forces new DevNonce generation)
    // Note: RadioLib automatically manages DevNonce and will use a new one on next join
    
    Serial.println(F("[LoRa] [SUCCESS] ✅ Persistence cleared - next join will use fresh DevNonce"));
    Serial.println(F("[LoRa] [INFO] Device will attempt to rejoin network with new credentials"));
}

void LoRaHandler::printStatus() {
    if (!initialized) {
        Serial.println(F("[LoRa] Status: Not initialized"));
        return;
    }
    
    Serial.println(F("[LoRa] =========================================="));
    Serial.println(F("[LoRa] Status Report"));
    Serial.println(F("[LoRa] =========================================="));
    Serial.printf("[LoRa] Initialized: %s\n", initialized ? "YES" : "NO");
    Serial.printf("[LoRa] Joined: %s\n", joined ? "YES" : "NO");
    Serial.printf("[LoRa] Last Error: %d (%s)\n", lastErrorCode, getErrorString(lastErrorCode).c_str());
    Serial.printf("[LoRa] Last RSSI: %.2f dBm\n", lastRssi);
    Serial.printf("[LoRa] Last SNR: %.2f dB\n", lastSnr);
    Serial.println(F("[LoRa] =========================================="));
}

void LoRaHandler::printNetworkInfo() {
    if (!initialized) {
        Serial.println(F("[LoRa] Network info: Not initialized"));
        return;
    }
    
    Serial.println(F("[LoRa] Network Configuration:"));
    Serial.println(F("[LoRa] - Band: US915"));
    Serial.println(F("[LoRa] - RX2 Frequency: 923.3 MHz"));
    Serial.println(F("[LoRa] - RX2 Data Rate: 8"));
}

void LoRaHandler::printJoinStatus() {
    if (!node) return;
    
    Serial.println(F("[LoRa] =========================================="));
    Serial.println(F("[LoRa] Join Status"));
    Serial.println(F("[LoRa] =========================================="));
    Serial.printf("[LoRa] Activated: %s\n", node->isActivated() ? "YES" : "NO");
    Serial.printf("[LoRa] RSSI: %.2f dBm\n", lastRssi);
    Serial.printf("[LoRa] SNR: %.2f dB\n", lastSnr);
    Serial.println(F("[LoRa] =========================================="));
}

void LoRaHandler::printCredentials() {
    Serial.println(F("[LoRa] Credentials (MSB format):"));
    Serial.print(F("[LoRa] DevEUI: "));
    for (int i = 0; i < 8; i++) {
        Serial.printf("%02X", DEVEUI[i]);
        if (i < 7) Serial.print(":");
    }
    Serial.println();
    
    Serial.print(F("[LoRa] AppEUI: "));
    for (int i = 0; i < 8; i++) {
        Serial.printf("%02X", APPEUI[i]);
        if (i < 7) Serial.print(":");
    }
    Serial.println();
    
    Serial.print(F("[LoRa] AppKey: "));
    for (int i = 0; i < 16; i++) {
        Serial.printf("%02X", APPKEY[i]);
        if (i < 15) Serial.print(":");
    }
    Serial.println();
}

String LoRaHandler::getErrorString(int16_t errorCode) {
    switch (errorCode) {
        case RADIOLIB_ERR_NONE:
            return F("No error");
        case RADIOLIB_ERR_CHIP_NOT_FOUND:
            return F("Chip not found");
        case RADIOLIB_ERR_PACKET_TOO_LONG:
            return F("Packet too long");
        case RADIOLIB_ERR_TX_TIMEOUT:
            return F("TX timeout");
        case RADIOLIB_ERR_RX_TIMEOUT:
            return F("RX timeout");
        case RADIOLIB_ERR_CRC_MISMATCH:
            return F("CRC mismatch");
        case RADIOLIB_ERR_INVALID_BANDWIDTH:
            return F("Invalid bandwidth");
        case RADIOLIB_ERR_INVALID_SPREADING_FACTOR:
            return F("Invalid spreading factor");
        case RADIOLIB_ERR_INVALID_CODING_RATE:
            return F("Invalid coding rate");
        case RADIOLIB_ERR_INVALID_FREQUENCY:
            return F("Invalid frequency");
        case RADIOLIB_ERR_INVALID_OUTPUT_POWER:
            return F("Invalid output power");
        case RADIOLIB_LORAWAN_SESSION_RESTORED:
            return F("Session restored");
        case RADIOLIB_LORAWAN_NEW_SESSION:
            return F("New session");
        case RADIOLIB_LORAWAN_NONCES_DISCARDED:
            return F("Nonces discarded");
        case RADIOLIB_LORAWAN_SESSION_DISCARDED:
            return F("Session discarded");
        default:
            return F("Unknown error");
    }
}

bool LoRaHandler::sendGatewayDiscoveryData(float latitude, float longitude, float altitude, int satellites, float rssi, float snr) {
    if (!initialized || !joined) {
        Serial.println(F("[LoRa] [ERROR] Not initialized or not joined"));
        return false;
    }
    
    // Create JSON payload for gateway discovery
    String payload = "{";
    payload += "\"type\":\"gateway_discovery\",";
    payload += "\"lat\":" + String(latitude, 6) + ",";
    payload += "\"lon\":" + String(longitude, 6) + ",";
    payload += "\"alt\":" + String(altitude, 1) + ",";
    payload += "\"sats\":" + String(satellites) + ",";
    payload += "\"rssi\":" + String(rssi, 1) + ",";
    payload += "\"snr\":" + String(snr, 1) + ",";
    payload += "\"timestamp\":" + String(millis());
    payload += "}";
    
    Serial.printf("[LoRa] [DISCOVERY] Sending gateway discovery data: %s\n", payload.c_str());
    
    // Send on port 4 for gateway discovery
    return sendData(payload, 4, true); // Use confirmed transmission for discovery data
}

void LoRaHandler::trackGatewayDiscovery(float latitude, float longitude, float altitude, int satellites) {
    if (!gatewayDiscoveryEnabled || !joined) {
        return;
    }
    
    // Check if we have valid GPS data
    if (satellites < 3 || latitude == 0.0 || longitude == 0.0) {
        return;
    }
    
    unsigned long currentTime = millis();
    
    // Check minimum interval between discoveries
    if (currentTime - lastGatewayDiscoveryTime < MIN_DISCOVERY_INTERVAL) {
        return;
    }
    
    // Get current signal quality
    float currentRssi = radio->getRSSI();
    float currentSnr = radio->getSNR();
    
    // Check for significant signal change or first discovery
    if (hasSignificantSignalChange(currentRssi, currentSnr) || lastGatewayDiscoveryTime == 0) {
        Serial.printf("[LoRa] [DISCOVERY] Gateway discovered! RSSI: %.1f dBm, SNR: %.1f dB\n", 
                      currentRssi, currentSnr);
        
        // Send discovery data
        if (sendGatewayDiscoveryData(latitude, longitude, altitude, satellites, currentRssi, currentSnr)) {
            lastGatewayRssi = currentRssi;
            lastGatewaySnr = currentSnr;
            lastGatewayDiscoveryTime = currentTime;
            Serial.println(F("[LoRa] [DISCOVERY] ✅ Gateway discovery data sent"));
        } else {
            Serial.println(F("[LoRa] [DISCOVERY] ❌ Failed to send gateway discovery data"));
        }
    }
}

bool LoRaHandler::hasSignificantSignalChange(float newRssi, float newSnr) {
    // First measurement
    if (lastGatewayRssi == -999.0) {
        return true;
    }
    
    // Check for significant RSSI change
    float rssiChange = abs(newRssi - lastGatewayRssi);
    if (rssiChange >= SIGNAL_CHANGE_THRESHOLD) {
        Serial.printf("[LoRa] [DISCOVERY] Significant RSSI change: %.1f -> %.1f dBm (Δ%.1f)\n", 
                      lastGatewayRssi, newRssi, rssiChange);
        return true;
    }
    
    return false;
}

void LoRaHandler::enableGatewayDiscovery(bool enable) {
    gatewayDiscoveryEnabled = enable;
    Serial.printf("[LoRa] [DISCOVERY] Gateway discovery %s\n", enable ? "enabled" : "disabled");
    
    if (enable) {
        // Reset tracking variables
        lastGatewayRssi = -999.0;
        lastGatewaySnr = -999.0;
        lastGatewayDiscoveryTime = 0;
    }
} 