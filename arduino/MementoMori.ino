// Memento Mori - Life in Weeks
// For reTerminal E1001 (800x480 e-paper, ESP32-S3)

#include <GxEPD2_BW.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSerif24pt7b.h>
#include <WiFi.h>
#include <time.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// Display configuration for reTerminal E1001
// 7.5" 800x480 monochrome e-paper
GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(GxEPD2_750_T7(/*CS=5*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));

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

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nMemento Mori - Initializing...");

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  // Load configuration
  loadConfig();

  // Initialize display
  display.init(115200);
  display.setRotation(0);  // Horizontal orientation
  display.setTextColor(GxEPD_BLACK);

  // Connect to WiFi and sync time
  syncTime();

  // Render display
  renderDisplay();

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
  config.specialDayCount = 0;
}

void syncTime() {
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
  if (!timeInitialized) {
    renderError("Time not synchronized");
    return;
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
    float dotRadius = min(cellWidth, cellHeight) / 3.0;

    // Draw circular dots
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        int weekIndex = row * COLS + col;

        int x = col * cellWidth + cellWidth / 2;
        int y = row * cellHeight + cellHeight / 2;

        if (weekIndex < weeksLived) {
          // Past: solid black circle
          display.fillCircle(x, y, dotRadius, GxEPD_BLACK);
        } else if (weekIndex < totalWeeks) {
          // Future: outline circle
          display.drawCircle(x, y, dotRadius, GxEPD_BLACK);
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

  // Enable timer wakeup
  esp_sleep_enable_timer_wakeup(secondsUntilMidnight * 1000000ULL);

  // Enter deep sleep
  Serial.println("Entering deep sleep...");
  Serial.flush();
  esp_deep_sleep_start();
}
