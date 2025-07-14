#ifndef GPS_HANDLER_H
#define GPS_HANDLER_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// Pin definitions for GPS (Heltec Wireless Tracker V1.1)
#define GPS_TX_PIN      33
#define GPS_RX_PIN      34
#define GPS_BAUD_RATE   9600

// GPS configuration constants
#define GPS_UPDATE_INTERVAL     1000    // Update GPS data every 1 second
#define GPS_TIMEOUT_MS          5000    // Timeout for GPS operations
#define GPS_MIN_SATELLITES      4       // Minimum satellites for valid fix

struct GPSData {
    bool isValid;
    float latitude;
    float longitude;
    float altitude;
    float speed;
    float course;
    int satellites;
    float hdop;
    unsigned long age;
    
    // Constructor
    GPSData() : isValid(false), latitude(0.0), longitude(0.0), altitude(0.0), 
                speed(0.0), course(0.0), satellites(0), hdop(0.0), age(0) {}
};

class GPSHandler {
private:
    TinyGPSPlus gps;
    HardwareSerial* gpsSerial;
    GPSData currentData;
    unsigned long lastUpdate;
    unsigned long lastValidFix;
    bool initialized;
    
    // Statistics
    unsigned long totalSentences;
    unsigned long failedChecksums;
    unsigned long passedChecksums;
    
    // Helper functions
    void updateGPSData();
    void printGPSStats();
    
public:
    GPSHandler();
    ~GPSHandler();
    
    // Initialization
    bool initialize();
    
    // Data acquisition
    void update();
    bool hasValidFix() const;
    bool hasNewData() const;
    GPSData getCurrentData() const { return currentData; }
    
    // Individual data getters
    float getLatitude() const { return currentData.latitude; }
    float getLongitude() const { return currentData.longitude; }
    float getAltitude() const { return currentData.altitude; }
    float getSpeed() const { return currentData.speed; }
    float getCourse() const { return currentData.course; }
    int getSatellites() const { return currentData.satellites; }
    float getHDOP() const { return currentData.hdop; }
    unsigned long getAge() const { return currentData.age; }
    
    // Status and diagnostics
    bool isInitialized() const { return initialized; }
    unsigned long getTimeSinceLastFix() const;
    String getStatusString() const;
    
    // Statistics
    unsigned long getTotalSentences() const { return totalSentences; }
    unsigned long getFailedChecksums() const { return failedChecksums; }
    unsigned long getPassedChecksums() const { return passedChecksums; }
    
    // Debug and status
    void printStatus();
    void printDetailedInfo();
    void printSatelliteInfo();
    
    // Utility functions
    String formatCoordinate(float coord, bool isLatitude) const;
    String formatTime() const;
    String formatDate() const;
    double distanceTo(float lat, float lon) const;
    double courseTo(float lat, float lon) const;
};

#endif // GPS_HANDLER_H 