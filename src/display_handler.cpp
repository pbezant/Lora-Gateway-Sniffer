#include "display_handler.h"

DisplayHandler::DisplayHandler() : display(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST),
                                  currentPage(PAGE_STATUS), 
                                  lastUpdate(0), lastPageSwitch(0), initialized(false),
                                  gpsFixed(false), gpsSatellites(0), gpsLatitude(0.0), gpsLongitude(0.0),
                                  loraJoined(false), loraRssi(0), loraSnr(0.0), loraStatus("Disconnected"),
                                  systemUptime(0), systemFreeHeap(0), systemCpuUsage(0.0), 
                                  systemBatteryVoltage(0.0), systemBatteryPercentage(0) {
    Serial.println(F("[Display] Handler created for Heltec Wireless Tracker V1.1"));
}

DisplayHandler::~DisplayHandler() {
    Serial.println(F("[Display] Handler destroyed"));
}

void DisplayHandler::controlBacklight(bool state) {
    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, state ? HIGH : LOW);
    Serial.printf("[Display] Backlight %s\n", state ? "ON" : "OFF");
}

bool DisplayHandler::initialize() {
    Serial.println(F("[Display] Initializing TFT LCD display..."));
    
    Serial.printf("[Display] Using pins - CS:%d, DC:%d, MOSI:%d, SCLK:%d, RST:%d, BLK:%d\n", 
                  TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST, TFT_BLK);
    
    // Initialize backlight FIRST - this is critical!
    controlBacklight(true);
    delay(100);
    
    // Initialize the display
    display.initR(INITR_MINI160x80);  // 160x80 pixel display
    
    // Set orientation and colors
    display.setRotation(1);  // Landscape orientation
    display.fillScreen(ST7735_BLACK);
    display.setTextColor(ST7735_WHITE);
    display.setTextSize(1);
    
    // Test display with simple message
    display.setCursor(10, 10);
    display.println("Heltec Tracker");
    display.setCursor(10, 20);
    display.println("Display Ready!");
    
    initialized = true;
    Serial.println(F("[Display] TFT LCD initialized successfully"));
    
    return true;
}

void DisplayHandler::update() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    
    // Auto-switch pages every 5 seconds
    if (currentTime - lastPageSwitch > 5000) {
        nextPage();
        lastPageSwitch = currentTime;
    }
    
    // Update display content every second
    if (currentTime - lastUpdate > DISPLAY_UPDATE_INTERVAL) {
        display.fillScreen(ST7735_BLACK);
        
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
        }
        
        lastUpdate = currentTime;
        Serial.printf("[Display] Updated page %d\n", currentPage);
    }
}

void DisplayHandler::nextPage() {
    currentPage = (DisplayPage)((currentPage + 1) % PAGE_COUNT);
    Serial.printf("[Display] Switched to page %d\n", currentPage);
}

void DisplayHandler::showMessage(const char* message) {
    if (!initialized) return;
    
    Serial.printf("[Display] Showing message: %s\n", message);
    
    display.fillScreen(ST7735_BLACK);
    display.setTextColor(ST7735_WHITE);
    display.setTextSize(1);
    
    // Center the message
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(message, 0, 0, &x1, &y1, &w, &h);
    
    int x = (DISPLAY_WIDTH - w) / 2;
    int y = (DISPLAY_HEIGHT - h) / 2;
    
    display.setCursor(x, y);
    display.println(message);
}

void DisplayHandler::showMessage(const String& message) {
    showMessage(message.c_str());
}

void DisplayHandler::showSuccess(const char* message) {
    if (!initialized) return;
    
    display.fillScreen(ST7735_BLACK);
    display.setTextColor(ST7735_GREEN);
    display.setTextSize(1);
    
    display.setCursor(10, 10);
    display.println("SUCCESS:");
    display.setCursor(10, 25);
    display.println(message);
}

void DisplayHandler::showError(const char* message) {
    if (!initialized) return;
    
    display.fillScreen(ST7735_BLACK);
    display.setTextColor(ST7735_RED);
    display.setTextSize(1);
    
    display.setCursor(10, 10);
    display.println("ERROR:");
    display.setCursor(10, 25);
    display.println(message);
}

void DisplayHandler::showError(const String& message) {
    showError(message.c_str());
}

void DisplayHandler::drawStatusPage() {
    display.setTextColor(ST7735_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("STATUS");
    
    display.setCursor(0, 15);
    display.printf("GPS: %s (%d sats)\n", gpsFixed ? "FIXED" : "SEARCH", gpsSatellites);
    
    display.setCursor(0, 30);
    display.printf("LoRa: %s\n", loraJoined ? "JOINED" : "DISCONN");
    
    display.setCursor(0, 45);
    display.printf("Uptime: %lus\n", systemUptime / 1000);
    
    display.setCursor(0, 60);
    display.printf("Battery: %d%%\n", systemBatteryPercentage);
}

void DisplayHandler::drawGPSPage() {
    display.setTextColor(ST7735_CYAN);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("GPS INFO");
    
    display.setTextColor(ST7735_WHITE);
    display.setCursor(0, 15);
    display.printf("Status: %s\n", gpsFixed ? "FIXED" : "SEARCHING");
    
    display.setCursor(0, 30);
    display.printf("Satellites: %d\n", gpsSatellites);
    
    if (gpsFixed) {
        display.setCursor(0, 45);
        display.printf("Lat: %.6f\n", gpsLatitude);
        display.setCursor(0, 60);
        display.printf("Lon: %.6f\n", gpsLongitude);
    }
}

void DisplayHandler::drawLoRaPage() {
    display.setTextColor(ST7735_YELLOW);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("LORA INFO");
    
    display.setTextColor(ST7735_WHITE);
    display.setCursor(0, 15);
    display.printf("Status: %s\n", loraStatus.c_str());
    
    display.setCursor(0, 30);
    display.printf("Joined: %s\n", loraJoined ? "YES" : "NO");
    
    display.setCursor(0, 45);
    display.printf("RSSI: %d dBm\n", loraRssi);
    
    display.setCursor(0, 60);
    display.printf("SNR: %.1f dB\n", loraSnr);
}

void DisplayHandler::drawSystemPage() {
    display.setTextColor(ST7735_MAGENTA);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("SYSTEM");
    
    display.setTextColor(ST7735_WHITE);
    display.setCursor(0, 15);
    display.printf("Uptime: %lus\n", systemUptime / 1000);
    
    display.setCursor(0, 30);
    display.printf("Free RAM: %lu\n", systemFreeHeap);
    
    display.setCursor(0, 45);
    display.printf("CPU: %.1f%%\n", systemCpuUsage);
    
    display.setCursor(0, 60);
    display.printf("Battery: %.2fV\n", systemBatteryVoltage);
}

void DisplayHandler::drawMessage(const char* message) {
    display.setTextColor(ST7735_WHITE);
    display.setTextSize(1);
    display.setCursor(10, 35);
    display.println(message);
}

void DisplayHandler::drawCenteredText(const char* text, int y) {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    
    int x = (DISPLAY_WIDTH - w) / 2;
    display.setCursor(x, y);
    display.println(text);
}

void DisplayHandler::updateGPSInfo(bool fixed, int satellites, double lat, double lon) {
    gpsFixed = fixed;
    gpsSatellites = satellites;
    gpsLatitude = lat;
    gpsLongitude = lon;
}

void DisplayHandler::updateLoRaInfo(bool joined, int rssi, float snr, const String& status) {
    loraJoined = joined;
    loraRssi = rssi;
    loraSnr = snr;
    loraStatus = status;
}

void DisplayHandler::updateSystemInfo(unsigned long uptime, unsigned long freeHeap, float cpuUsage, 
                                     float batteryVoltage, int batteryPercentage) {
    systemUptime = uptime;
    systemFreeHeap = freeHeap;
    systemCpuUsage = cpuUsage;
    systemBatteryVoltage = batteryVoltage;
    systemBatteryPercentage = batteryPercentage;
}

void DisplayHandler::printStatus() {
    Serial.printf("[Display] Status - Page: %d, Initialized: %s\n", 
                  currentPage, initialized ? "YES" : "NO");
} 