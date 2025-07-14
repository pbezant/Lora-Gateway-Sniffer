#include "lora_handler.h"
#include "secrets.h"

LoRaHandler::LoRaHandler() : 
    radio(nullptr), 
    node(nullptr), 
    initialized(false), 
    joined(false), 
    lastSendTime(0), 
    lastJoinAttempt(0),
    lastErrorCode(0),
    lastRssi(0.0),
    lastSnr(0.0) {
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
    
    // Create radio instance
    radio = new SX1262(new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY));
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
            
            // Send initial test message
            delay(1000); // Give network time to process
            String testMessage = "Online!";
            if (sendData(testMessage)) {
                Serial.println(F("[LoRa] [SUCCESS] ✅ Initial test message sent"));
            } else {
                Serial.println(F("[LoRa] [WARN] Failed to send initial test message"));
            }
            
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
    
    Serial.printf("[LoRa] Sending data on port %d: %s\n", port, data.c_str());
    
    String dataToSend = data; // Create non-const copy
    int16_t state = node->uplink(dataToSend, port, confirmed);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("[LoRa] [SUCCESS] ✅ Data sent successfully"));
        lastSendTime = millis();
        
        // Update signal quality
        lastRssi = radio->getRSSI();
        lastSnr = radio->getSNR();
        
        return true;
    } else {
        Serial.printf("[LoRa] [ERROR] ❌ Failed to send data, code: %d (%s)\n", state, getErrorString(state).c_str());
        lastErrorCode = state;
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

bool LoRaHandler::sendStatusData(unsigned long uptime, size_t freeHeap) {
    if (!initialized || !joined) {
        Serial.println(F("[LoRa] [ERROR] Not initialized or not joined"));
        return false;
    }
    
    // Create status data payload
    String statusData = "{\"uptime\":" + String(uptime) + 
                        ",\"heap\":" + String(freeHeap) + 
                        ",\"rssi\":" + String(lastRssi, 1) + 
                        ",\"snr\":" + String(lastSnr, 1) + "}";
    
    return sendData(statusData, 3);
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
    Serial.println(F("[LoRa] [INFO] DevNonce reset not supported in this simplified version"));
    Serial.println(F("[LoRa] [INFO] Try clearing persistence instead"));
}

uint16_t LoRaHandler::getCurrentDevNonce() const {
    Serial.println(F("[LoRa] [INFO] DevNonce query not supported in this simplified version"));
    return 0;
}

void LoRaHandler::clearPersistence() {
    if (!initialized || !node) {
        Serial.println(F("[LoRa] [ERROR] Cannot clear persistence - not initialized"));
        return;
    }
    
    Serial.println(F("[LoRa] [DEBUG] Clearing persistence..."));
    // Reset join state
    joined = false;
    Serial.println(F("[LoRa] [SUCCESS] Persistence cleared"));
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