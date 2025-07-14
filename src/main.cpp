#include <Arduino.h>
#include "display_handler.h"
#include "gps_handler.h"
#include "lora_handler.h"

// Global handler instances
DisplayHandler displayHandler;
GPSHandler gpsHandler;
LoRaHandler loraHandler;

// Application state
enum AppState {
    STATE_INITIALIZING,
    STATE_RUNNING,
    STATE_ERROR
};

AppState currentState = STATE_INITIALIZING;
String lastError = "";

// Timing variables
unsigned long lastStatusUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastLoRaSend = 0;
unsigned long bootTime = 0;

// Constants
const unsigned long PERIODIC_INTERVAL = 120; // Send data every 2 minutes (120 seconds) - respects LoRaWAN duty cycle

// Function prototypes
void initializeSystem();
void initializeDisplay();
void initializeGPS();
void initializeLoRa();
void handleMainLoop();
void handleError(const String& error);
void updateSystemStatus();
void sendPeriodicData();
void printSystemInfo();
void onJoinAccept();

// Helper function to read battery voltage from GPIO 15
float readBatteryVoltage() {
    // GPIO 15 is the confirmed battery voltage pin for Heltec Wireless Tracker v1.1
    const int BATTERY_PIN = 15;
    
    uint32_t reading = analogReadMilliVolts(BATTERY_PIN);
    float voltage = (2.0f * reading) / 1000.0f; // Assume 2:1 voltage divider
    
    Serial.printf("[MAIN] Battery voltage on GPIO %d: %.3f V\n", BATTERY_PIN, voltage);
    return voltage;
}

// Helper function to convert battery voltage to percentage
float batteryVoltageToPercentage(float voltage) {
    // LiPo battery voltage ranges (adjust these based on your battery)
    const float BATTERY_MIN = 3.0f; // Empty battery voltage
    const float BATTERY_MAX = 4.2f; // Full battery voltage
    
    if (voltage >= BATTERY_MAX) return 100.0f;
    if (voltage <= BATTERY_MIN) return 0.0f;
    
    return ((voltage - BATTERY_MIN) / (BATTERY_MAX - BATTERY_MIN)) * 100.0f;
}

void setup() {
    Serial.begin(115200);
    delay(2000); // Wait for serial to initialize
    
    bootTime = millis();
    
    Serial.println(F("\n=== LoRa Gateway Sniffer ==="));
    Serial.println(F("Heltec Wireless Tracker v1.1"));
    Serial.println(F("ESP32-S3 with SX1262 LoRa"));
    Serial.println(F("===============================\n"));
    
    // Initialize system components
    initializeSystem();
    
    Serial.println(F("\n[MAIN] System initialization complete"));
    Serial.println(F("[MAIN] Entering main loop...\n"));
    
    currentState = STATE_RUNNING;
}

void loop() {
    switch (currentState) {
        case STATE_INITIALIZING:
            // Should not reach here after setup
            delay(1000);
            break;
            
        case STATE_RUNNING:
            handleMainLoop();
            break;
            
        case STATE_ERROR:
            Serial.printf("[MAIN] [ERROR] System in error state: %s\n", lastError.c_str());
            Serial.println(F("[MAIN] [ERROR] Attempting recovery in 10 seconds..."));
            delay(10000);
            
            // Attempt to recover
            Serial.println(F("[MAIN] [INFO] Attempting system recovery..."));
            initializeSystem();
            currentState = STATE_RUNNING;
            break;
    }
    
    // Small delay to prevent tight loop
    delay(100);
}

void initializeSystem() {
    Serial.println(F("[MAIN] Starting system initialization..."));
    
    // Initialize display first for visual feedback
    initializeDisplay();
    
    // Initialize GPS
    initializeGPS();

    // Initialize LoRa
    initializeLoRa();
    
    // Print initial system information
    printSystemInfo();
}

void initializeDisplay() {
    Serial.println(F("[MAIN] Initializing display..."));
    
    if (!displayHandler.initialize()) {
        handleError("Display initialization failed");
        return;
    }
    
    displayHandler.showMessage("Display OK", 1000);
    Serial.println(F("[MAIN] [SUCCESS] Display initialized"));
}

void initializeGPS() {
    Serial.println(F("[MAIN] Initializing GPS..."));
    displayHandler.showMessage("Starting GPS...", 0);
    
    if (!gpsHandler.initialize()) {
        handleError("GPS initialization failed");
        return;
    }
    
    displayHandler.showMessage("GPS Started", 1000);
    Serial.println(F("[MAIN] [SUCCESS] GPS initialized"));
}

void initializeLoRa() {
    Serial.println(F("[MAIN] Initializing LoRa..."));
    displayHandler.showMessage("Starting LoRa...", 0);
    
    // Initialize LoRa hardware
    if (!loraHandler.initialize()) {
        handleError("LoRa hardware initialization failed");
        return;
    }
    
    displayHandler.showMessage("LoRa HW OK", 1000);
    
    // Configure credentials
    displayHandler.showMessage("Configuring...", 0);
    if (!loraHandler.configureCredentials()) {
        handleError("LoRa credential configuration failed");
        return;
    }
    
    displayHandler.showMessage("Config OK", 1000);
    
    // Attempt to join network
    displayHandler.showMessage("Joining network...", 0);
    if (!loraHandler.joinNetwork()) {
        // Don't treat join failure as fatal error, it might succeed later
        Serial.println(F("[MAIN] [WARN] LoRa network join failed, will retry later"));
        displayHandler.showMessage("Join failed", 2000);
    } else {
        displayHandler.showSuccess("LoRa Joined!", 2000);
        Serial.println(F("[MAIN] [SUCCESS] LoRa network joined"));
    }
    
    Serial.println(F("[MAIN] [SUCCESS] LoRa initialized"));
}

void handleMainLoop() {
    // Handle serial commands
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();
        
        if (command == "reset_devnonce" || command == "rd") {
            Serial.println(F("[MAIN] [CMD] Resetting DevNonce..."));
            loraHandler.resetDevNonce();
        } else if (command == "rejoin" || command == "rj") {
            Serial.println(F("[MAIN] [CMD] Attempting to rejoin network..."));
            loraHandler.joinNetwork();
        } else if (command == "status" || command == "s") {
            Serial.println(F("[MAIN] [CMD] System status:"));
            printSystemInfo();
            loraHandler.printStatus();
        } else if (command == "devnonce" || command == "dn") {
            Serial.printf("[MAIN] [CMD] Current DevNonce: %u (0x%04X)\n", 
                         loraHandler.getCurrentDevNonce(), loraHandler.getCurrentDevNonce());
        } else if (command == "clear_persistence" || command == "cp") {
            Serial.println(F("[MAIN] [CMD] Clearing persistence..."));
            loraHandler.clearPersistence();
        } else if (command == "enable_discovery" || command == "ed") {
            Serial.println(F("[MAIN] [CMD] Enabling gateway discovery..."));
            loraHandler.enableGatewayDiscovery(true);
        } else if (command == "disable_discovery" || command == "dd") {
            Serial.println(F("[MAIN] [CMD] Disabling gateway discovery..."));
            loraHandler.enableGatewayDiscovery(false);
        } else if (command == "help" || command == "h") {
            Serial.println(F("[MAIN] [CMD] Available commands:"));
            Serial.println(F("[MAIN] [CMD] - reset_devnonce (rd): Reset DevNonce and force fresh join"));
            Serial.println(F("[MAIN] [CMD] - rejoin (rj): Attempt to rejoin LoRaWAN network"));
            Serial.println(F("[MAIN] [CMD] - status (s): Show system status"));
            Serial.println(F("[MAIN] [CMD] - devnonce (dn): Show DevNonce info"));
            Serial.println(F("[MAIN] [CMD] - clear_persistence (cp): Clear session data (RECOMMENDED for -1108 errors)"));
            Serial.println(F("[MAIN] [CMD] - enable_discovery (ed): Enable automatic gateway discovery"));
            Serial.println(F("[MAIN] [CMD] - disable_discovery (dd): Disable automatic gateway discovery"));
            Serial.println(F("[MAIN] [CMD] - help (h): Show this help"));
        } else if (command.length() > 0) {
            Serial.printf("[MAIN] [CMD] Unknown command: %s (type 'help' for available commands)\n", command.c_str());
        }
    }

    // Update GPS data
    gpsHandler.update();

    // Update display periodically
    if (millis() - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL) {
        updateSystemStatus();
        displayHandler.update();
        lastDisplayUpdate = millis();
    }
    
    // Handle LoRa periodic tasks (reconnection attempts, etc.)
    loraHandler.handlePeriodicTasks();

    // Send periodic data if LoRa is connected
    if (loraHandler.isJoined() && (millis() - lastLoRaSend > PERIODIC_INTERVAL)) {
        sendPeriodicData();
        lastLoRaSend = millis();
    }
    
    // Print system status periodically
    if (millis() - lastStatusUpdate > 30000) {
        printSystemInfo();
        lastStatusUpdate = millis();
    }
}

void handleError(const String& error) {
    Serial.printf("[MAIN] [ERROR] %s\n", error.c_str());
    lastError = error;
    currentState = STATE_ERROR;
    
    // Show error on display
    displayHandler.showError(error, 3000);
}

void updateSystemStatus() {
    // Update display with current system information
    unsigned long uptime = millis() - bootTime;
    size_t freeHeap = ESP.getFreeHeap();
    float batteryVoltage = readBatteryVoltage();
    float batteryPercentage = batteryVoltageToPercentage(batteryVoltage);
    
    // Update system info (pass both voltage and percentage)
    displayHandler.updateSystemInfo(uptime, freeHeap, 0.0, batteryVoltage, batteryPercentage);
    
    // Update GPS status
    if (gpsHandler.hasValidFix()) {
        GPSData gpsData = gpsHandler.getCurrentData();
        displayHandler.updateGPSInfo(true, gpsData.satellites, gpsData.latitude, gpsData.longitude);
    } else {
        displayHandler.updateGPSInfo(false, gpsHandler.getSatelliteCount(), 0.0, 0.0);
    }
    
    // Update LoRa status
    displayHandler.updateLoRaInfo(loraHandler.isJoined(), loraHandler.getLastRssi(), loraHandler.getLastSnr(), loraHandler.isJoined() ? "Connected" : "Disconnected");
}

void sendPeriodicData() {
    Serial.println(F("[MAIN] Sending periodic data..."));
    
    // Get current data
    unsigned long uptime = millis() - bootTime;
    size_t freeHeap = ESP.getFreeHeap();
    float batteryVoltage = readBatteryVoltage();
    float batteryPercentage = batteryVoltageToPercentage(batteryVoltage);
    
    // Get GPS data
    bool hasGPS = gpsHandler.hasValidFix();
    float lat = 0.0, lon = 0.0, alt = 0.0;
    int sats = 0;
    
    if (hasGPS) {
        GPSData gpsData = gpsHandler.getCurrentData();
        lat = gpsData.latitude;
        lon = gpsData.longitude;
        alt = gpsData.altitude;
        sats = gpsData.satellites;
    }
    
    // Send combined status + GPS + battery data
    if (loraHandler.sendStatusData(uptime, freeHeap, batteryVoltage, batteryPercentage, hasGPS, lat, lon, alt, sats)) {
        Serial.printf("[MAIN] Combined data sent successfully (Battery: %.3f V, %.1f%%, GPS: %s)\n", 
                     batteryVoltage, batteryPercentage, hasGPS ? "Valid" : "No fix");
    } else {
        Serial.println(F("[MAIN] Failed to send combined data"));
    }
}

void printSystemInfo() {
    Serial.println(F("\n[MAIN] === System Status Report ==="));
    Serial.printf("[MAIN] Uptime: %lu seconds\n", (millis() - bootTime) / 1000);
    Serial.printf("[MAIN] Free heap: %zu bytes\n", ESP.getFreeHeap());
    Serial.printf("[MAIN] Chip model: %s\n", ESP.getChipModel());
    Serial.printf("[MAIN] CPU frequency: %lu MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("[MAIN] Flash size: %lu bytes\n", ESP.getFlashChipSize());
    
    // Print handler status
    Serial.println(F("\n[MAIN] === Handler Status ==="));
    Serial.printf("[MAIN] Display: %s\n", displayHandler.isInitialized() ? "OK" : "ERROR");
    Serial.printf("[MAIN] GPS: %s\n", gpsHandler.isInitialized() ? "OK" : "ERROR");
    Serial.printf("[MAIN] LoRa: %s\n", loraHandler.isInitialized() ? "OK" : "ERROR");
    
    // Print detailed status from each handler
    gpsHandler.printStatus();
    loraHandler.printStatus();
    displayHandler.printStatus();
    
    Serial.println(F("[MAIN] === End Status Report ===\n"));
}

void onJoinAccept() {
    Serial.println(F("[MAIN] ✅ Join accepted!"));
    displayHandler.updateLoRaInfo(true, 0, 0.0, "Connected");
    // Remove the "Online!" packet - just set the flag to start periodic data
    // No initial packet sent here anymore
}