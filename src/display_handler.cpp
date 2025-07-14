#include "display_handler.h"

DisplayHandler::DisplayHandler() : display(nullptr), currentPage(PAGE_STATUS), 
                                  lastUpdate(0), lastPageSwitch(0), initialized(false) {
    Serial.println(F("[Display] Handler created"));
}

DisplayHandler::~DisplayHandler() {
    if (display) {
        delete display;
        display = nullptr;
    }
    Serial.println(F("[Display] Handler destroyed"));
}

void DisplayHandler::resetDisplay() {
    Serial.println(F("[Display] Performing hardware reset..."));
    
    // Configure reset pin
    pinMode(OLED_RST, OUTPUT);
    
    // Reset sequence: LOW -> delay -> HIGH -> delay
    digitalWrite(OLED_RST, LOW);
    delay(10);
    digitalWrite(OLED_RST, HIGH);
    delay(10);
    
    Serial.println(F("[Display] Hardware reset completed"));
}

bool DisplayHandler::initialize() {
    Serial.println(F("[Display] Initializing display handler..."));
    
    Serial.printf("[Display] Using pins - SDA:%d, SCL:%d, RST:%d\n", 
                  OLED_SDA, OLED_SCL, OLED_RST);
    
    // Perform hardware reset first
    resetDisplay();

    // Use software I2C. The U8G2 library will handle the low-level communication.
    display = new U8G2_SSD1306_128X64_NONAME_F_SW_I2C(U8G2_R0, OLED_SCL, OLED_SDA, OLED_RST);

    if (!display) {
        Serial.println(F("[Display] [ERROR] Failed to create display instance"));
        return false;
    }
    
    // Attempt to initialize at the default I2C address
    Serial.println(F("[Display] Trying display->begin() at default address 0x3C..."));
    if (display->begin()) {
        Serial.println(F("[Display] [SUCCESS] Display initialized at 0x3C"));
        initialized = true;

        // Perform basic setup before drawing anything
        display->setFont(u8g2_font_6x10_tf); // A simple, reliable font
        display->setDrawColor(1);            // Set drawing color to ON

        setContrast(10); // Set a low but visible contrast
        showMessage("Display OK", 1000);
        return true;
    }

    // If that fails, try the alternate I2C address
    Serial.println(F("[Display] Begin failed at 0x3C. Trying alternate address 0x3D..."));
    display->setI2CAddress(0x3D);
    if (display->begin()) {
        Serial.println(F("[Display] [SUCCESS] Display initialized at 0x3D"));
        initialized = true;

        // Perform basic setup before drawing anything
        display->setFont(u8g2_font_6x10_tf); // A simple, reliable font
        display->setDrawColor(1);            // Set drawing color to ON

        setContrast(10); // Set a low but visible contrast
        showMessage("Display OK", 1000);
        return true;
    }

    Serial.println(F("[Display] [FATAL] display->begin() failed at both 0x3C and 0x3D. Check hardware."));
    delete display;
    display = nullptr;
    return false;
}

void DisplayHandler::setContrast(uint8_t contrast) {
    if (!initialized || !display) return;
    display->setContrast(contrast);
}

void DisplayHandler::update() {
    if (!initialized || !display) return;
    
    if (millis() - lastUpdate < DISPLAY_UPDATE_INTERVAL) {
        return;
    }
    lastUpdate = millis();

    if (millis() - lastPageSwitch > 5000) {
        nextPage();
    }
    
    display->clearBuffer();
    
    switch (currentPage) {
        case PAGE_STATUS:
            drawStatusPage();
            break;
        case PAGE_GPS:
            drawGPSPage();
            break;
        case PAGE_LORA:
            drawLoRaPage();
            break;
        case PAGE_SYSTEM:
            drawSystemPage();
            break;
        default:
            drawMessage("Unknown Page");
            break;
    }
    
    display->sendBuffer();
}

void DisplayHandler::drawStatusPage() {
    drawHeader("Status");
    
    // Device status
    display->drawStr(0, 25, "Device:");
    display->drawStr(45, 25, displayData.deviceStatus.c_str());
    
    // GPS status
    display->drawStr(0, 35, "GPS:");
    display->drawStr(45, 35, displayData.gpsStatus.c_str());
    
    // LoRa status
    display->drawStr(0, 45, "LoRa:");
    display->drawStr(45, 45, displayData.loraStatus.c_str());
    
    // System info
    display->drawStr(0, 55, "Uptime:");
    display->drawStr(45, 55, formatUptime(displayData.systemUptime).c_str());
}

void DisplayHandler::drawGPSPage() {
    drawHeader("GPS");
    
    // GPS fix status
    display->drawStr(0, 25, displayData.gpsHasFix ? "Fix: YES" : "Fix: NO");
    
    // Satellite count with signal bars
    char satStr[16];
    snprintf(satStr, sizeof(satStr), "Sats: %d", displayData.gpsSatellites);
    display->drawStr(0, 35, satStr);
    drawSignalBars(70, 28, displayData.gpsSatellites, 12);
    
    // Coordinates (if available)
    if (displayData.gpsHasFix) {
        char latStr[32], lonStr[32];
        snprintf(latStr, sizeof(latStr), "Lat: %.4f", displayData.gpsLatitude);
        snprintf(lonStr, sizeof(lonStr), "Lon: %.4f", displayData.gpsLongitude);
        display->drawStr(0, 45, latStr);
        display->drawStr(0, 55, lonStr);
    } else {
        display->drawStr(0, 45, "Waiting for");
        display->drawStr(0, 55, "satellite fix...");
    }
}

void DisplayHandler::drawLoRaPage() {
    drawHeader("LoRa");
    
    // Join status
    display->drawStr(0, 25, displayData.loraJoined ? "Joined: YES" : "Joined: NO");
    
    if (displayData.loraJoined) {
        // RSSI
        char rssiStr[16];
        snprintf(rssiStr, sizeof(rssiStr), "RSSI: %d dBm", displayData.loraRssi);
        display->drawStr(0, 35, rssiStr);
        
        // SNR
        char snrStr[16];
        snprintf(snrStr, sizeof(snrStr), "SNR: %.1f dB", displayData.loraSnr);
        display->drawStr(0, 45, snrStr);
        
        // Signal quality bars based on RSSI
        int signalBars = 0;
        if (displayData.loraRssi > -80) signalBars = 4;
        else if (displayData.loraRssi > -90) signalBars = 3;
        else if (displayData.loraRssi > -100) signalBars = 2;
        else if (displayData.loraRssi > -110) signalBars = 1;
        
        drawSignalBars(90, 38, signalBars, 4);
    } else {
        display->drawStr(0, 35, "Status:");
        display->drawStr(0, 45, displayData.loraStatus.c_str());
    }
}

void DisplayHandler::drawSystemPage() {
    drawHeader("System");
    
    // Uptime
    display->drawStr(0, 20, "Uptime:");
    display->drawStr(60, 20, formatUptime(displayData.systemUptime).c_str());
    
    // Free heap
    display->drawStr(0, 32, "Heap:");
    display->drawStr(60, 32, formatMemory(displayData.systemFreeHeap).c_str());
    
    // Memory usage bar
    size_t totalHeap = 327680; // Approximate total heap for ESP32-S3
    int memUsage = 100 - (displayData.systemFreeHeap * 100 / totalHeap);
    drawProgressBar(0, 45, 100, 8, memUsage);
    
    // Temperature (if available)
    if (displayData.systemTemperature > 0) {
        char tempStr[16];
        snprintf(tempStr, sizeof(tempStr), "%.1f C", displayData.systemTemperature);
        display->drawStr(0, 44, "Temp:");
        display->drawStr(60, 44, tempStr);
    }
    // Battery Voltage and Percentage
    display->drawStr(0, 56, "Batt:");
    char battStr[32];
    snprintf(battStr, sizeof(battStr), "%.2fV %.0f%%", displayData.batteryVoltage, displayData.batteryPercentage);
    display->drawStr(60, 56, battStr);
}

void DisplayHandler::drawHeader(const char* title) {
    // Draw title
    display->setFont(u8g2_font_7x13_tf);
    display->drawStr(0, 12, title);
    
    // Draw page indicator
    char pageStr[16];
    snprintf(pageStr, sizeof(pageStr), "%d/%d", currentPage + 1, PAGE_COUNT);
    display->drawStr(DISPLAY_WIDTH - getTextWidth(pageStr), 12, pageStr);
    
    // Draw separator line
    display->drawLine(0, 15, DISPLAY_WIDTH, 15);
    
    // Reset font
    display->setFont(u8g2_font_6x10_tf);
}

void DisplayHandler::drawProgressBar(int x, int y, int width, int height, int percentage) {
    // Draw border
    display->drawFrame(x, y, width, height);
    
    // Draw fill
    int fillWidth = (width - 2) * percentage / 100;
    if (fillWidth > 0) {
        display->drawBox(x + 1, y + 1, fillWidth, height - 2);
    }
    
    // Draw percentage text
    char percentStr[8];
    snprintf(percentStr, sizeof(percentStr), "%d%%", percentage);
    int textX = x + (width - getTextWidth(percentStr)) / 2;
    display->setDrawColor(percentage > 50 ? 0 : 1); // Invert text if bar is dark
    display->drawStr(textX, y + height - 1, percentStr);
    display->setDrawColor(1); // Reset draw color
}

void DisplayHandler::drawSignalBars(int x, int y, int bars, int maxBars) {
    int barWidth = 3;
    int barSpacing = 1;
    int maxHeight = 10;
    
    for (int i = 0; i < maxBars; i++) {
        int barHeight = (maxHeight * (i + 1)) / maxBars;
        int barX = x + i * (barWidth + barSpacing);
        int barY = y + maxHeight - barHeight;
        
        if (i < bars) {
            display->drawBox(barX, barY, barWidth, barHeight);
        } else {
            display->drawFrame(barX, barY, barWidth, barHeight);
        }
    }
}

String DisplayHandler::formatUptime(unsigned long uptime) {
    unsigned long seconds = uptime / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    char buffer[32];
    if (days > 0) {
        snprintf(buffer, sizeof(buffer), "%lud %02lu:%02lu", days, hours % 24, minutes % 60);
    } else {
        snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", hours, minutes % 60, seconds % 60);
    }
    
    return String(buffer);
}

String DisplayHandler::formatMemory(size_t bytes) {
    char buffer[16];
    if (bytes >= 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.1fMB", bytes / (1024.0 * 1024.0));
    } else if (bytes >= 1024) {
        snprintf(buffer, sizeof(buffer), "%.1fKB", bytes / 1024.0);
    } else {
        snprintf(buffer, sizeof(buffer), "%zuB", bytes);
    }
    return String(buffer);
}

void DisplayHandler::clear() {
    if (!initialized || !display) return;
    display->clearBuffer();
    display->sendBuffer();
}

void DisplayHandler::turnOn() {
    if (!initialized || !display) return;
    display->setPowerSave(0);
}

void DisplayHandler::turnOff() {
    if (!initialized || !display) return;
    display->setPowerSave(1);
}

void DisplayHandler::setBrightness(uint8_t brightness) {
    if (!initialized || !display) return;
    display->setContrast(brightness);
}

void DisplayHandler::nextPage() {
    if (!initialized || !display) return;
    currentPage = static_cast<DisplayPage>((currentPage + 1) % PAGE_COUNT);
    lastPageSwitch = millis();
}

void DisplayHandler::previousPage() {
    currentPage = (DisplayPage)((currentPage - 1 + PAGE_COUNT) % PAGE_COUNT);
}

void DisplayHandler::setPage(DisplayPage page) {
    if (page < PAGE_COUNT) {
        currentPage = page;
    }
}

void DisplayHandler::updateStatus(const String& status) {
    displayData.deviceStatus = status;
}

void DisplayHandler::updateGPSInfo(bool hasfix, int satellites, float latitude, float longitude) {
    displayData.gpsHasFix = hasfix;
    displayData.gpsSatellites = satellites;
    displayData.gpsLatitude = latitude;
    displayData.gpsLongitude = longitude;
    displayData.gpsStatus = hasfix ? "Fix OK" : "No fix";
}

void DisplayHandler::updateLoRaInfo(bool joined, int rssi, float snr, const String& status) {
    displayData.loraJoined = joined;
    displayData.loraRssi = rssi;
    displayData.loraSnr = snr;
    displayData.loraStatus = status;
}

void DisplayHandler::updateSystemInfo(unsigned long uptime, size_t freeHeap, float temperature, float batteryVoltage, float batteryPercentage) {
    displayData.systemUptime = uptime;
    displayData.systemFreeHeap = freeHeap;
    displayData.systemTemperature = temperature;
    displayData.batteryVoltage = batteryVoltage;
    displayData.batteryPercentage = batteryPercentage;
}

void DisplayHandler::showMessage(const String& message, int duration) {
    if (!initialized || !display) return;
    
    display->clearBuffer();
    drawCenteredText(message.c_str(), 32);
    display->sendBuffer();
    
    if (duration > 0) {
        delay(duration);
    }
}

void DisplayHandler::showError(const String& error, int duration) {
    if (!initialized || !display) return;
    
    display->clearBuffer();
    drawCenteredText("ERROR", 20);
    drawCenteredText(error.c_str(), 35);
    display->sendBuffer();
    
    if (duration > 0) {
        delay(duration);
    }
}

void DisplayHandler::showSuccess(const String& message, int duration) {
    if (!initialized || !display) return;
    
    display->clearBuffer();
    drawCenteredText("SUCCESS", 20);
    drawCenteredText(message.c_str(), 35);
    display->sendBuffer();
    
    if (duration > 0) {
        delay(duration);
    }
}

void DisplayHandler::showProgress(const String& message, int percentage) {
    if (!initialized || !display) return;
    
    display->clearBuffer();
    drawCenteredText(message.c_str(), 25);
    drawProgressBar(20, 35, 88, 10, percentage);
    display->sendBuffer();
}

void DisplayHandler::printStatus() {
    if (!initialized) {
        Serial.println(F("[Display] Status: Not initialized"));
        return;
    }
    
    Serial.println(F("[Display] === Display Status ==="));
    Serial.printf("[Display] Initialized: %s\n", initialized ? "Yes" : "No");
    Serial.printf("[Display] Current page: %d\n", currentPage);
    Serial.printf("[Display] Last update: %lu ms ago\n", millis() - lastUpdate);
}

void DisplayHandler::drawMessage(const char* message) {
    display->setFont(u8g2_font_helvR14_tf);
    display->drawStr(0, 15, message);
}

void DisplayHandler::drawCenteredText(const char* text, int y) {
    if (!display) return;
    int textWidth = getTextWidth(text);
    int x = (DISPLAY_WIDTH - textWidth) / 2;
    display->drawStr(x, y, text);
}

void DisplayHandler::drawRightAlignedText(const char* text, int y) {
    if (!display) return;
    int textWidth = getTextWidth(text);
    int x = DISPLAY_WIDTH - textWidth;
    display->drawStr(x, y, text);
}

int DisplayHandler::getTextWidth(const char* text) {
    if (!display) return 0;
    return display->getStrWidth(text);
} 