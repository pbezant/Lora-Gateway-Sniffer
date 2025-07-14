#ifndef DISPLAY_HANDLER_H
#define DISPLAY_HANDLER_H

#include <Arduino.h>
#include <U8g2lib.h>

// Pin definitions for OLED display (Heltec Wireless Tracker V1.1)
#define OLED_SDA    17
#define OLED_SCL    18
#define OLED_RST    21

// Display configuration constants
#define DISPLAY_WIDTH       128
#define DISPLAY_HEIGHT      64
#define DISPLAY_UPDATE_INTERVAL 1000  // Update display every 1 second

// Display page enumeration
enum DisplayPage {
    PAGE_STATUS = 0,
    PAGE_GPS,
    PAGE_LORA,
    PAGE_SYSTEM,
    PAGE_COUNT  // Total number of pages
};

class DisplayHandler {
private:
    U8G2_SSD1306_128X64_NONAME_F_SW_I2C* display;
    DisplayPage currentPage;
    unsigned long lastUpdate;
    unsigned long lastPageSwitch;
    bool initialized;
    
    // Display content methods
    void drawStatusPage();
    void drawGPSPage();
    void drawLoRaPage();
    void drawSystemPage();
    
    // Helper methods
    void drawHeader(const char* title);
    void drawProgressBar(int x, int y, int width, int height, int percentage);
    void drawSignalBars(int x, int y, int bars, int maxBars);
    String formatUptime(unsigned long uptime);
    String formatMemory(size_t bytes);
    
public:
    DisplayHandler();
    ~DisplayHandler();
    
    // Initialization
    bool initialize();
    
    // Display control
    void update();
    void clear();
    void turnOn();
    void turnOff();
    void setBrightness(uint8_t brightness);
    
    // Page management
    void nextPage();
    void previousPage();
    void setPage(DisplayPage page);
    DisplayPage getCurrentPage() const { return currentPage; }
    
    // Content update methods
    void updateStatus(const String& status);
    void updateGPSInfo(bool hasfix, int satellites, float latitude, float longitude);
    void updateLoRaInfo(bool joined, int rssi, float snr, const String& status);
    void updateSystemInfo(unsigned long uptime, size_t freeHeap, float temperature = 0.0);
    
    // Direct display methods
    void showMessage(const String& message, int duration = 2000);
    void showError(const String& error, int duration = 3000);
    void showSuccess(const String& message, int duration = 2000);
    void showProgress(const String& message, int percentage);
    
    // Status and diagnostics
    bool isInitialized() const { return initialized; }
    void printStatus();
    
    // Utility methods
    void drawCenteredText(const char* text, int y);
    void drawRightAlignedText(const char* text, int y);
    int getTextWidth(const char* text);
    
private:
    // Cached display data
    struct DisplayData {
        // Status data
        String deviceStatus;
        
        // GPS data
        bool gpsHasFix;
        int gpsSatellites;
        float gpsLatitude;
        float gpsLongitude;
        String gpsStatus;
        
        // LoRa data
        bool loraJoined;
        int loraRssi;
        float loraSnr;
        String loraStatus;
        
        // System data
        unsigned long systemUptime;
        size_t systemFreeHeap;
        float systemTemperature;
        
        DisplayData() : deviceStatus("Starting..."), gpsHasFix(false), gpsSatellites(0),
                       gpsLatitude(0.0), gpsLongitude(0.0), gpsStatus("No fix"),
                       loraJoined(false), loraRssi(0), loraSnr(0.0), loraStatus("Not connected"),
                       systemUptime(0), systemFreeHeap(0), systemTemperature(0.0) {}
    } displayData;
};

#endif // DISPLAY_HANDLER_H 