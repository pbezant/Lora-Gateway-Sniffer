#include "gps_handler.h"

GPSHandler::GPSHandler() : gpsSerial(nullptr), lastUpdate(0), lastValidFix(0), initialized(false),
                          totalSentences(0), failedChecksums(0), passedChecksums(0) {
    Serial.println(F("[GPS] Handler created"));
}

GPSHandler::~GPSHandler() {
    if (gpsSerial) {
        gpsSerial->end();
        gpsSerial = nullptr;
    }
    Serial.println(F("[GPS] Handler destroyed"));
}

bool GPSHandler::initialize() {
    Serial.println(F("[GPS] Initializing GPS handler..."));
    
    // Initialize GPS serial communication
    gpsSerial = &Serial1;
    gpsSerial->begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    
    if (!gpsSerial) {
        Serial.println(F("[GPS] [ERROR] Failed to initialize GPS serial"));
        return false;
    }
    
    Serial.printf("[GPS] GPS serial initialized on pins RX:%d, TX:%d at %d baud\n", 
                  GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD_RATE);
    
    // Clear any existing data
    while (gpsSerial->available()) {
        gpsSerial->read();
    }
    
    initialized = true;
    lastUpdate = millis();
    
    Serial.println(F("[GPS] [SUCCESS] GPS handler initialized"));
    Serial.println(F("[GPS] [INFO] Waiting for satellite signals..."));
    Serial.println(F("[GPS] [INFO] This may take several minutes outdoors"));
    
    return true;
}

void GPSHandler::update() {
    if (!initialized || !gpsSerial) return;
    
    // Process incoming GPS data
    while (gpsSerial->available()) {
        char c = gpsSerial->read();
        if (gps.encode(c)) {
            updateGPSData();
        }
    }
    
    // Update statistics
    totalSentences = gps.sentencesWithFix() + gps.failedChecksum();
    failedChecksums = gps.failedChecksum();
    passedChecksums = gps.passedChecksum();
    
    // Periodic status updates
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 30000) { // Every 30 seconds
        printStatus();
        lastStatusPrint = millis();
    }
}

void GPSHandler::updateGPSData() {
    if (!initialized) return;
    
    // Check if we have a valid location fix
    if (gps.location.isValid()) {
        currentData.isValid = true;
        currentData.latitude = gps.location.lat();
        currentData.longitude = gps.location.lng();
        currentData.age = gps.location.age();
        lastValidFix = millis();
        
        Serial.printf("[GPS] [SUCCESS] Valid fix: %.6f, %.6f (age: %lu ms)\n", 
                      currentData.latitude, currentData.longitude, currentData.age);
    } else {
        currentData.isValid = false;
    }
    
    // Update altitude if available
    if (gps.altitude.isValid()) {
        currentData.altitude = gps.altitude.meters();
    }
    
    // Update speed if available
    if (gps.speed.isValid()) {
        currentData.speed = gps.speed.kmph();
    }
    
    // Update course if available
    if (gps.course.isValid()) {
        currentData.course = gps.course.deg();
    }
    
    // Update satellite count
    if (gps.satellites.isValid()) {
        currentData.satellites = gps.satellites.value();
    }
    
    // Update HDOP (horizontal dilution of precision)
    if (gps.hdop.isValid()) {
        currentData.hdop = gps.hdop.hdop();
    }
    
    lastUpdate = millis();
}

bool GPSHandler::hasValidFix() const {
    return initialized && currentData.isValid && 
           (currentData.satellites >= GPS_MIN_SATELLITES) &&
           (millis() - lastValidFix < GPS_TIMEOUT_MS);
}

bool GPSHandler::hasNewData() const {
    return initialized && (millis() - lastUpdate < GPS_UPDATE_INTERVAL);
}

unsigned long GPSHandler::getTimeSinceLastFix() const {
    if (lastValidFix == 0) return ULONG_MAX;
    return millis() - lastValidFix;
}

String GPSHandler::getStatusString() const {
    if (!initialized) return "Not initialized";
    if (!currentData.isValid) return "No fix";
    if (currentData.satellites < GPS_MIN_SATELLITES) return "Insufficient satellites";
    if (getTimeSinceLastFix() > GPS_TIMEOUT_MS) return "Fix timeout";
    return "Valid fix";
}

void GPSHandler::printStatus() {
    if (!initialized) {
        Serial.println(F("[GPS] Status: Not initialized"));
        return;
    }
    
    Serial.println(F("[GPS] === GPS Status ==="));
    Serial.printf("[GPS] Initialized: %s\n", initialized ? "Yes" : "No");
    Serial.printf("[GPS] Valid fix: %s\n", currentData.isValid ? "Yes" : "No");
    Serial.printf("[GPS] Satellites: %d\n", currentData.satellites);
    Serial.printf("[GPS] Status: %s\n", getStatusString().c_str());
    
    if (currentData.isValid) {
        Serial.printf("[GPS] Location: %.6f, %.6f\n", currentData.latitude, currentData.longitude);
        Serial.printf("[GPS] Altitude: %.2f m\n", currentData.altitude);
        Serial.printf("[GPS] Speed: %.2f km/h\n", currentData.speed);
        Serial.printf("[GPS] Course: %.2f°\n", currentData.course);
        Serial.printf("[GPS] HDOP: %.2f\n", currentData.hdop);
        Serial.printf("[GPS] Age: %lu ms\n", currentData.age);
    }
    
    Serial.printf("[GPS] Time since last fix: %lu ms\n", getTimeSinceLastFix());
    printGPSStats();
}

void GPSHandler::printDetailedInfo() {
    if (!initialized) {
        Serial.println(F("[GPS] Detailed info: Not initialized"));
        return;
    }
    
    Serial.println(F("[GPS] === Detailed GPS Information ==="));
    
    // Location information
    Serial.printf("[GPS] Location valid: %s\n", gps.location.isValid() ? "Yes" : "No");
    if (gps.location.isValid()) {
        Serial.printf("[GPS] Latitude: %.8f\n", gps.location.lat());
        Serial.printf("[GPS] Longitude: %.8f\n", gps.location.lng());
        Serial.printf("[GPS] Location age: %lu ms\n", gps.location.age());
    }
    
    // Date and time
    if (gps.date.isValid() && gps.time.isValid()) {
        Serial.printf("[GPS] Date: %s\n", formatDate().c_str());
        Serial.printf("[GPS] Time: %s\n", formatTime().c_str());
    }
    
    // Altitude
    Serial.printf("[GPS] Altitude valid: %s\n", gps.altitude.isValid() ? "Yes" : "No");
    if (gps.altitude.isValid()) {
        Serial.printf("[GPS] Altitude: %.2f m\n", gps.altitude.meters());
    }
    
    // Speed
    Serial.printf("[GPS] Speed valid: %s\n", gps.speed.isValid() ? "Yes" : "No");
    if (gps.speed.isValid()) {
        Serial.printf("[GPS] Speed: %.2f km/h\n", gps.speed.kmph());
    }
    
    // Course
    Serial.printf("[GPS] Course valid: %s\n", gps.course.isValid() ? "Yes" : "No");
    if (gps.course.isValid()) {
        Serial.printf("[GPS] Course: %.2f°\n", gps.course.deg());
    }
    
    printSatelliteInfo();
}

void GPSHandler::printSatelliteInfo() {
    Serial.println(F("[GPS] === Satellite Information ==="));
    Serial.printf("[GPS] Satellites valid: %s\n", gps.satellites.isValid() ? "Yes" : "No");
    if (gps.satellites.isValid()) {
        Serial.printf("[GPS] Satellites in view: %d\n", gps.satellites.value());
    }
    
    Serial.printf("[GPS] HDOP valid: %s\n", gps.hdop.isValid() ? "Yes" : "No");
    if (gps.hdop.isValid()) {
        Serial.printf("[GPS] HDOP: %.2f\n", gps.hdop.hdop());
    }
}

void GPSHandler::printGPSStats() {
    Serial.println(F("[GPS] === GPS Statistics ==="));
    Serial.printf("[GPS] Total sentences: %lu\n", totalSentences);
    Serial.printf("[GPS] Passed checksums: %lu\n", passedChecksums);
    Serial.printf("[GPS] Failed checksums: %lu\n", failedChecksums);
    Serial.printf("[GPS] Characters processed: %lu\n", gps.charsProcessed());
    
    if (totalSentences > 0) {
        float successRate = (float)passedChecksums / totalSentences * 100.0;
        Serial.printf("[GPS] Success rate: %.1f%%\n", successRate);
    }
}

String GPSHandler::formatCoordinate(float coord, bool isLatitude) const {
    char buffer[32];
    char direction = ' ';
    
    if (isLatitude) {
        direction = (coord >= 0) ? 'N' : 'S';
    } else {
        direction = (coord >= 0) ? 'E' : 'W';
    }
    
    float absCoord = abs(coord);
    int degrees = (int)absCoord;
    float minutes = (absCoord - degrees) * 60.0;
    
    snprintf(buffer, sizeof(buffer), "%d°%.4f'%c", degrees, minutes, direction);
    return String(buffer);
}

String GPSHandler::formatTime() const {
    if (!gps.time.isValid()) return "Invalid";
    
    char buffer[16];
    // Cast away const since TinyGPS++ methods are not const-qualified
    TinyGPSPlus& mutableGps = const_cast<TinyGPSPlus&>(gps);
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", 
             mutableGps.time.hour(), mutableGps.time.minute(), mutableGps.time.second());
    return String(buffer);
}

String GPSHandler::formatDate() const {
    if (!gps.date.isValid()) return "Invalid";
    
    char buffer[16];
    // Cast away const since TinyGPS++ methods are not const-qualified
    TinyGPSPlus& mutableGps = const_cast<TinyGPSPlus&>(gps);
    snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", 
             mutableGps.date.month(), mutableGps.date.day(), mutableGps.date.year());
    return String(buffer);
}

double GPSHandler::distanceTo(float lat, float lon) const {
    if (!currentData.isValid) return 0.0;
    return gps.distanceBetween(currentData.latitude, currentData.longitude, lat, lon);
}

double GPSHandler::courseTo(float lat, float lon) const {
    if (!currentData.isValid) return 0.0;
    return gps.courseTo(currentData.latitude, currentData.longitude, lat, lon);
} 