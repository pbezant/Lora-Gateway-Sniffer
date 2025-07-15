#ifndef LORA_HANDLER_H
#define LORA_HANDLER_H

#include <Arduino.h>
#include <RadioLib.h>
#include "Config.h"
#include <Preferences.h>

struct LoRaSession {
    uint32_t devAddr;
    uint8_t nwkSKey[16];
    uint8_t appSKey[16];
    uint32_t fCntUp;
    uint32_t fCntDown;
    bool joined;
};

class LoRaHandler {
private:
    // RadioLib objects
    SX1262* radio;
    LoRaWANNode* node;
    
    // Status flags
    bool initialized;
    bool joined;
    
    // Timing
    unsigned long lastSendTime;
    unsigned long lastJoinAttempt;
    
    // Error handling
    int16_t lastErrorCode;
    
    // Signal quality
    float lastRssi;
    float lastSnr;
    
    // Internal methods
    void printJoinStatus();
    void printCredentials();
    Preferences nvs;
    LoRaSession session;
    void saveLoRaSession();
    bool loadLoRaSession();
    void clearLoRaSession();
    
public:
    LoRaHandler();
    ~LoRaHandler();
    
    // Initialization and setup
    bool initialize();
    bool configureCredentials();
    bool joinNetwork();
    
    // DevNonce and persistence management
    void resetDevNonce();
    uint16_t getCurrentDevNonce() const;
    void clearPersistence();
    
    // Data transmission
    bool sendData(const String& data, uint8_t port = 1, bool confirmed = false);
    bool sendGPSData(float latitude, float longitude, float altitude, int satellites);
    bool sendStatusData(unsigned long uptime, size_t freeHeap, float batteryVoltage, float batteryPercentage, bool hasGPS, float lat, float lon, float alt, int sats);
    bool sendGatewayDiscoveryData(float latitude, float longitude, float altitude, int satellites, float rssi, float snr);
    
    // Status and monitoring
    bool isJoined() const { return joined; }
    bool isInitialized() const { return initialized; }
    int16_t getLastError() const { return lastErrorCode; }
    float getLastRssi() const { return lastRssi; }
    float getLastSnr() const { return lastSnr; }
    
    // Periodic operations
    void handlePeriodicTasks();
    bool shouldSendData() const;
    
    // Error handling and debugging
    void printStatus();
    void printNetworkInfo();
    String getErrorString(int16_t errorCode);
    
    // Gateway discovery tracking
    void trackGatewayDiscovery(float latitude, float longitude, float altitude, int satellites);
    bool hasSignificantSignalChange(float newRssi, float newSnr);
    void enableGatewayDiscovery(bool enable = true);
    
    // Gateway discovery tracking variables
    bool gatewayDiscoveryEnabled;
    float lastGatewayRssi;
    float lastGatewaySnr;
    unsigned long lastGatewayDiscoveryTime;
    static const float SIGNAL_CHANGE_THRESHOLD;
    static const unsigned long MIN_DISCOVERY_INTERVAL;
};

#endif // LORA_HANDLER_H 