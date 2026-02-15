// Memento Mori - Life in Weeks
// For reTerminal E1001 (800x480 e-paper, ESP32-S3)

#include <GxEPD2_BW.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSerif24pt7b.h>
#include <WiFi.h>
#include <time.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <ArduinoOTA.h>

// Display configuration for reTerminal E1001
// 7.5" 800x480 monochrome e-paper
// Pin mapping from Seeed documentation: CS=10, DC=11, RST=12, BUSY=13
GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(GxEPD2_750_T7(/*CS=*/ 10, /*DC=*/ 11, /*RST=*/ 12, /*BUSY=*/ 13));

// Display constants
const int COLS = 52;  // weeks per year
const int ROWS = 80;  // years of life
const int DISPLAY_WIDTH = 800;
const int DISPLAY_HEIGHT = 480;

// Configuration structure
struct Config {
  String birthdate;  // Format: "YYYY-MM-DD"
  int expectedLifespan;
  String timezone;
  String wifiSSID;
  String wifiPassword;
  String ntpServer;

  struct SpecialDay {
    String date;   // Format: "MM-DD"
    String title;
    String quote;
  };

  SpecialDay specialDays[10];  // Support up to 10 special days
  int specialDayCount;
} config;

// Time tracking
struct tm timeinfo;
bool timeInitialized = false;

// OTA mode flag
bool otaMode = false;

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nMemento Mori - Initializing...");

  // Configure button pins - GPIO3 used for ext0 wakeup, don't reconfigure it
  // pinMode(3, INPUT_PULLUP);  // REMOVED - ext0 wakeup handles GPIO3 configuration
  pinMode(4, INPUT_PULLUP);  // Right white
  pinMode(5, INPUT_PULLUP);  // Left white

  delay(50);  // Let GPIO settle after wake from deep sleep

  // Check wake cause to determine if woken by button or timer
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Woke from button press (EXT0)");

    // Check if button STILL held (for OTA mode)
    delay(100);  // Debounce
    if (digitalRead(3) == LOW) {
      Serial.println("Button still held, checking for OTA mode...");
      delay(1500);  // Wait 1.5 seconds to ensure intentional hold
      if (digitalRead(3) == LOW) {
        otaMode = true;
        Serial.println("OTA Mode activated via button hold");
      } else {
        Serial.println("Quick press detected, normal mode");
      }
    } else {
      Serial.println("Button released, normal wake");
    }
  } else if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Woke from timer (midnight update)");
  } else {
    Serial.println("First boot or reset");
  }

  // Initialize SPIFFS (make non-blocking)
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed - using defaults");
    setDefaultConfig();
  } else {
    // Load configuration
    loadConfig();
  }

  // Initialize display
  display.init(115200);
  display.setRotation(0);  // Horizontal orientation
  display.setTextColor(GxEPD_BLACK);

  // Connect to WiFi and sync time
  syncTime();

  // Render display
  renderDisplay();

  // Configure wake on button press (GPIO3 = green button)
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_3, 0);  // Wake when button pressed (LOW)

  // Enter deep sleep until tomorrow midnight
  enterDeepSleep();
}

void loop() {
  // Never reached - device resets after deep sleep
}

void loadConfig() {
  Serial.println("Loading configuration...");

  File file = SPIFFS.open("/config.json", "r");
  if (!file) {
    Serial.println("Failed to open config file, using defaults");
    setDefaultConfig();
    return;
  }

  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.print("Failed to parse config: ");
    Serial.println(error.c_str());
    setDefaultConfig();
    return;
  }

  // Parse person config
  config.birthdate = doc["person"]["birthdate"].as<String>();
  config.expectedLifespan = doc["person"]["expectedLifespan"] | 80;
  config.timezone = doc["person"]["timezone"] | "America/New_York";

  // Parse WiFi config
  config.wifiSSID = doc["wifi"]["ssid"].as<String>();
  config.wifiPassword = doc["wifi"]["password"].as<String>();
  config.ntpServer = doc["wifi"]["ntpServer"] | "pool.ntp.org";

  // Parse special days
  JsonArray specialDays = doc["specialDays"];
  config.specialDayCount = min((int)specialDays.size(), 10);

  for (int i = 0; i < config.specialDayCount; i++) {
    config.specialDays[i].date = specialDays[i]["date"].as<String>();
    config.specialDays[i].title = specialDays[i]["title"].as<String>();
    config.specialDays[i].quote = specialDays[i]["quote"].as<String>();
  }

  Serial.println("Configuration loaded successfully");
  Serial.print("Birthdate: ");
  Serial.println(config.birthdate);
  Serial.print("Special days: ");
  Serial.println(config.specialDayCount);
}

void setDefaultConfig() {
  config.birthdate = "1987-08-17";
  config.expectedLifespan = 80;
  config.timezone = "America/New_York";
  config.wifiSSID = "YOUR_SSID";
  config.wifiPassword = "YOUR_PASSWORD";
  config.ntpServer = "pool.ntp.org";
  config.specialDayCount = 0;
}

void setupOTA() {
  ArduinoOTA.setHostname("memento-mori");
  ArduinoOTA.setPassword("memento2024");

  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("OTA Update: " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Complete");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
  });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void syncTime() {
  // Skip WiFi if no credentials
  if (config.wifiSSID.length() == 0) {
    Serial.println("No WiFi credentials - skipping time sync");
    return;
  }

  Serial.print("Connecting to WiFi: ");
  Serial.println(config.wifiSSID);

  WiFi.begin(config.wifiSSID.c_str(), config.wifiPassword.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connection failed");
    return;
  }

  Serial.println("\nWiFi connected");

  // Setup OTA
  setupOTA();

  // If OTA mode, wait for updates (60 seconds)
  if (otaMode) {
    Serial.println("Waiting for OTA updates (60 seconds)...");
    unsigned long startTime = millis();
    while (millis() - startTime < 60000) {
      ArduinoOTA.handle();
      delay(10);
    }
    Serial.println("OTA window closed");
  }

  // Configure timezone and NTP
  configTime(0, 0, config.ntpServer.c_str());  // UTC time
  setenv("TZ", config.timezone.c_str(), 1);
  tzset();

  // Wait for time sync
  Serial.print("Syncing time");
  attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts < 10) {
    Serial.print(".");
    delay(1000);
    attempts++;
  }

  if (attempts < 10) {
    Serial.println("\nTime synchronized");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    timeInitialized = true;
  } else {
    Serial.println("\nTime sync failed");
  }

  // Disconnect WiFi to save power
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

int calculateWeeksLived(const String& birthdate, struct tm* now) {
  // Parse birthdate
  int birthYear, birthMonth, birthDay;
  sscanf(birthdate.c_str(), "%d-%d-%d", &birthYear, &birthMonth, &birthDay);

  // Calculate age (birthday-based)
  int age = now->tm_year + 1900 - birthYear;

  // Check if birthday has occurred this year
  if (now->tm_mon + 1 < birthMonth ||
      (now->tm_mon + 1 == birthMonth && now->tm_mday < birthDay)) {
    age--;  // Birthday hasn't happened yet
  }

  // Calculate last birthday
  struct tm lastBirthday = *now;
  lastBirthday.tm_year = now->tm_year + 1900 - 1900;  // Current year
  lastBirthday.tm_mon = birthMonth - 1;
  lastBirthday.tm_mday = birthDay;

  if (now->tm_mon + 1 < birthMonth ||
      (now->tm_mon + 1 == birthMonth && now->tm_mday < birthDay)) {
    lastBirthday.tm_year--;  // Use last year's birthday
  }

  // Calculate weeks since last birthday
  time_t nowTime = mktime(now);
  time_t lastBirthdayTime = mktime(&lastBirthday);
  int daysSinceLastBirthday = (nowTime - lastBirthdayTime) / (60 * 60 * 24);
  int weeksSinceLastBirthday = daysSinceLastBirthday / 7;

  return (age * 52) + weeksSinceLastBirthday;
}

int checkSpecialDay(struct tm* now) {
  char today[6];
  snprintf(today, sizeof(today), "%02d-%02d", now->tm_mon + 1, now->tm_mday);

  for (int i = 0; i < config.specialDayCount; i++) {
    if (config.specialDays[i].date.equals(today)) {
      return i;  // Return index of special day
    }
  }

  return -1;  // No special day
}

void renderDisplay() {
  // If time not initialized, use hardcoded date for testing (Feb 14, 2026)
  if (!timeInitialized) {
    Serial.println("Using hardcoded date for testing: Feb 14, 2026");
    timeinfo.tm_year = 2026 - 1900;  // Years since 1900
    timeinfo.tm_mon = 1;             // February (0-indexed)
    timeinfo.tm_mday = 14;           // 14th
    timeinfo.tm_hour = 12;
    timeinfo.tm_min = 0;
    timeinfo.tm_sec = 0;
  }

  int weeksLived = calculateWeeksLived(config.birthdate, &timeinfo);
  int totalWeeks = config.expectedLifespan * 52;
  int specialDayIndex = checkSpecialDay(&timeinfo);

  Serial.print("Weeks lived: ");
  Serial.print(weeksLived);
  Serial.print(" of ");
  Serial.println(totalWeeks);

  if (specialDayIndex >= 0) {
    Serial.print("Special day: ");
    Serial.println(config.specialDays[specialDayIndex].title);
    renderSpecialDay(config.specialDays[specialDayIndex]);
  } else {
    renderGrid(weeksLived, totalWeeks);
  }
}

void renderGrid(int weeksLived, int totalWeeks) {
  display.setFullWindow();
  display.firstPage();

  do {
    display.fillScreen(GxEPD_WHITE);

    // Calculate cell dimensions
    float cellWidth = (float)DISPLAY_WIDTH / COLS;
    float cellHeight = (float)DISPLAY_HEIGHT / ROWS;
    float dotSize = min(cellWidth, cellHeight) * 0.7;  // 70% of cell size

    // Draw square dots
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        int weekIndex = row * COLS + col;

        int x = col * cellWidth + (cellWidth - dotSize) / 2;
        int y = row * cellHeight + (cellHeight - dotSize) / 2;

        if (weekIndex < weeksLived) {
          // Past: solid black square
          display.fillRect(x, y, dotSize, dotSize, GxEPD_BLACK);
        } else if (weekIndex < totalWeeks) {
          // Future: outline square
          display.drawRect(x, y, dotSize, dotSize, GxEPD_BLACK);
        }
      }
    }

  } while (display.nextPage());

  Serial.println("Grid rendered");
}

void renderSpecialDay(Config::SpecialDay& specialDay) {
  display.setFullWindow();
  display.firstPage();

  do {
    display.fillScreen(GxEPD_WHITE);

    // Center the quote
    display.setFont(&FreeSerif24pt7b);
    display.setTextColor(GxEPD_BLACK);

    // Word wrap and center the quote
    int16_t x = DISPLAY_WIDTH / 2;
    int16_t y = DISPLAY_HEIGHT / 2;

    // Simple word wrap (basic implementation)
    String quote = specialDay.quote;
    int maxWidth = DISPLAY_WIDTH - 120;

    // Draw centered text
    drawCenteredText(quote, y);

  } while (display.nextPage());

  Serial.println("Special day rendered");
}

void drawCenteredText(String text, int y) {
  // Simple implementation - draw text centered
  // For production, implement proper word wrapping
  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((DISPLAY_WIDTH - w) / 2, y);
  display.print(text);
}

void renderError(const char* message) {
  display.setFullWindow();
  display.firstPage();

  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeSerif18pt7b);
    display.setTextColor(GxEPD_BLACK);
    drawCenteredText(String(message), DISPLAY_HEIGHT / 2);
  } while (display.nextPage());

  Serial.print("Error: ");
  Serial.println(message);
}

void enterDeepSleep() {
  if (!timeInitialized) {
    Serial.println("Time not initialized, sleeping for 1 hour");
    esp_sleep_enable_timer_wakeup(3600ULL * 1000000ULL);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_3, 0);  // Wake on green button
    esp_deep_sleep_start();
    return;
  }

  // Calculate seconds until next midnight
  int currentHour = timeinfo.tm_hour;
  int currentMin = timeinfo.tm_min;
  int currentSec = timeinfo.tm_sec;

  int secondsUntilMidnight = (23 - currentHour) * 3600 +
                              (59 - currentMin) * 60 +
                              (60 - currentSec);

  Serial.print("Sleeping for ");
  Serial.print(secondsUntilMidnight);
  Serial.println(" seconds until midnight");

  // Enable wake on timer (midnight) AND button press
  esp_sleep_enable_timer_wakeup(secondsUntilMidnight * 1000000ULL);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_3, 0);  // Wake on green button

  // Enter deep sleep
  Serial.println("Entering deep sleep...");
  Serial.flush();
  esp_deep_sleep_start();
}
