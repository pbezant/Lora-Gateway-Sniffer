#ifndef DISPLAY_HANDLER_H
#define DISPLAY_HANDLER_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "Config.h"

// Display dimensions for Heltec Wireless Tracker V1.1
#ifndef DISPLAY_WIDTH
#define DISPLAY_WIDTH  160
#endif
#ifndef DISPLAY_HEIGHT
#define DISPLAY_HEIGHT 80
#endif

// Display update interval
#define DISPLAY_UPDATE_INTERVAL 1000

// Display pages
enum DisplayPage {
    PAGE_STATUS = 0,
    PAGE_GPS = 1,
    PAGE_LORA = 2,
    PAGE_SYSTEM = 3,
    PAGE_COUNT = 4
};

class DisplayHandler {
private:
    Adafruit_ST7735 display;
    DisplayPage currentPage;
    unsigned long lastUpdate;
    unsigned long lastPageSwitch;
    bool initialized;
    
    // System state variables
    bool gpsFixed;
    int gpsSatellites;
    double gpsLatitude;
    double gpsLongitude;
    
    bool loraJoined;
    int loraRssi;
    float loraSnr;
    String loraStatus;
    
    unsigned long systemUptime;
    unsigned long systemFreeHeap;
    float systemCpuUsage;
    float systemBatteryVoltage;
    int systemBatteryPercentage;
    
    // Display content methods
    void drawStatusPage();
    void drawGPSPage();
    void drawLoRaPage();
    void drawSystemPage();
    void drawMessage(const char* message);
    void drawCenteredText(const char* text, int y);
    
    // Backlight control
    void controlBacklight(bool state);

public:
    DisplayHandler();
    ~DisplayHandler();
    
    bool initialize();
    void update();
    void nextPage();
    void showMessage(const char* message);
    void showMessage(const String& message);
    void showSuccess(const char* message);
    void showError(const char* message);
    void showError(const String& message);
    
    // System info update methods
    void updateGPSInfo(bool fixed, int satellites, double lat, double lon);
    void updateLoRaInfo(bool joined, int rssi, float snr, const String& status);
    void updateSystemInfo(unsigned long uptime, unsigned long freeHeap, float cpuUsage, 
                         float batteryVoltage, int batteryPercentage);
    
    // Status method for main.cpp compatibility
    void printStatus();
    
    bool isInitialized() const { return initialized; }
    DisplayPage getCurrentPage() const { return currentPage; }
    void enableDisplayPower();
    void disableDisplayPower();
};

#endif // DISPLAY_HANDLER_H 