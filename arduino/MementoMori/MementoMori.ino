// Memento Mori - Life in Weeks
// For reTerminal E1001 (480×800 e-paper, Vertical Orientation)
// Minimalist design: centered title, dot-based battery indicator, edge-to-edge grid

#include <GxEPD2_BW.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSerif24pt7b.h>
#include <WiFi.h>
#include <time.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <driver/rtc_io.h>

// Display configuration for reTerminal E1001
// 7.5" 800x480 monochrome e-paper
// Pin mapping from Seeed documentation: CS=10, DC=11, RST=12, BUSY=13
GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(GxEPD2_750_T7(/*CS=*/ 10, /*DC=*/ 11, /*RST=*/ 12, /*BUSY=*/ 13));

// Display constants - HORIZONTAL ORIENTATION (800×480)
const int COLS = 52;  // weeks per year (horizontal)
const int ROWS = 80;  // years of life (vertical)
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
  String posixTZ;  // POSIX timezone string (e.g., "EST5EDT,M3.2.0,M11.1.0")

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

  // Configure GPIO3 for ext0 wakeup using RTC GPIO functions
  // This ensures proper pull resistor configuration for wake-from-sleep
  rtc_gpio_init(GPIO_NUM_3);
  rtc_gpio_set_direction(GPIO_NUM_3, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_dis(GPIO_NUM_3);   // Disable pull-down
  rtc_gpio_pullup_en(GPIO_NUM_3);      // Enable pull-up (button pulls to ground)
  rtc_gpio_hold_dis(GPIO_NUM_3);       // Release from hold if held

  pinMode(4, INPUT_PULLUP);  // Right white
  pinMode(5, INPUT_PULLUP);  // Left white

  delay(50);  // Let GPIO settle after wake from deep sleep

  // Check wake cause
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Woke from button press (EXT0)");
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
  display.setRotation(1);  // Horizontal orientation (800×480)
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

  // Try local config first (contains real WiFi creds, not checked into git)
  File file = SPIFFS.open("/config.local.json", "r");
  if (file) {
    Serial.println("Using config.local.json");
  } else {
    file = SPIFFS.open("/config.json", "r");
  }
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

  // Parse POSIX timezone string (required for ESP32 setenv TZ)
  config.posixTZ = doc["person"]["posixTZ"] | "EST5EDT,M3.2.0,M11.1.0";

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
  config.posixTZ = "EST5EDT,M3.2.0,M11.1.0";
  config.wifiSSID = "";
  config.wifiPassword = "";
  config.ntpServer = "pool.ntp.org";
  config.specialDayCount = 0;
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

  // Configure timezone and NTP
  // Use POSIX TZ string (e.g., "EST5EDT,M3.2.0,M11.1.0"), not Olson names
  configTime(0, 0, config.ntpServer.c_str());
  setenv("TZ", config.posixTZ.c_str(), 1);
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

  // Calculate last birthday (normalize to noon to avoid time-of-day errors)
  struct tm lastBirthday = {0};
  lastBirthday.tm_year = now->tm_year;  // Current year (years since 1900)
  lastBirthday.tm_mon = birthMonth - 1;
  lastBirthday.tm_mday = birthDay;
  lastBirthday.tm_hour = 12;
  lastBirthday.tm_isdst = -1;

  if (now->tm_mon + 1 < birthMonth ||
      (now->tm_mon + 1 == birthMonth && now->tm_mday < birthDay)) {
    lastBirthday.tm_year--;  // Use last year's birthday
  }

  // Normalize 'now' to noon as well for consistent day counting
  struct tm nowNoon = *now;
  nowNoon.tm_hour = 12;
  nowNoon.tm_min = 0;
  nowNoon.tm_sec = 0;
  nowNoon.tm_isdst = -1;

  // Calculate weeks since last birthday
  time_t nowTime = mktime(&nowNoon);
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

int getBatteryPercent() {
  // TODO: Implement actual battery reading for reTerminal E1001
  // For now, return 75% as placeholder
  return 75;
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
  int batteryPercent = getBatteryPercent();

  Serial.print("Weeks lived: ");
  Serial.print(weeksLived);
  Serial.print(" of ");
  Serial.print(totalWeeks);
  Serial.print(" | Battery: ");
  Serial.print(batteryPercent);
  Serial.println("%");

  if (specialDayIndex >= 0) {
    Serial.print("Special day: ");
    Serial.println(config.specialDays[specialDayIndex].title);
    renderSpecialDay(config.specialDays[specialDayIndex], batteryPercent);
  } else {
    renderGrid(weeksLived, totalWeeks, batteryPercent);
  }
}

// Draw WiFi status indicator (top-left corner)
void drawWiFiStatus() {
  if (timeInitialized) {
    // Black dot = WiFi/time sync succeeded
    display.fillCircle(12, 16, 3, GxEPD_BLACK);
  }
  // No dot = time sync failed (using hardcoded date)
}

// Draw battery indicator as dots (0-5 dots based on percentage)
void drawBatteryDots(int batteryPercent) {
  int numDots = (batteryPercent + 19) / 20;  // 1-20% = 1 dot, 21-40% = 2 dots, etc.
  if (numDots > 5) numDots = 5;
  if (numDots < 0) numDots = 0;

  int dotRadius = 2;
  int dotSpacing = 7;
  int x = DISPLAY_WIDTH - 12;
  int y = 16;

  for (int i = 0; i < numDots; i++) {
    display.fillCircle(x - (i * dotSpacing), y, dotRadius, GxEPD_BLACK);
  }
}

void renderGrid(int weeksLived, int totalWeeks, int batteryPercent) {
  display.setFullWindow();
  display.firstPage();

  do {
    display.fillScreen(GxEPD_WHITE);

    // Grid bleeds to edges - vertical design
    const int topMargin = 36;
    const int bottomMargin = 12;

    const int gridY = topMargin;
    const int gridHeight = DISPLAY_HEIGHT - topMargin - bottomMargin;
    const float cellHeight = (float)gridHeight / ROWS;
    const float cellWidth = (float)DISPLAY_WIDTH / COLS;
    const float dotRadius = min(cellWidth, cellHeight) / 3.2;

    // Draw circular dots
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        int weekIndex = row * COLS + col;

        int x = (col * cellWidth) + cellWidth / 2;
        int y = gridY + (row * cellHeight) + cellHeight / 2;

        if (weekIndex < weeksLived) {
          // Past: filled black circle
          display.fillCircle(x, y, dotRadius, GxEPD_BLACK);
        } else if (weekIndex < totalWeeks) {
          // Future: outline circle (very light)
          display.drawCircle(x, y, dotRadius, GxEPD_BLACK);
        }
      }
    }

    // Centered title at top
    display.setFont(NULL);  // Use default font for clean minimal text
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(2);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds("MEMENTO MORI", 0, 0, &x1, &y1, &w, &h);
    display.setCursor((DISPLAY_WIDTH - w) / 2, 12);
    display.print("MEMENTO MORI");

    // WiFi status in top left
    drawWiFiStatus();

    // Battery dots in top right
    drawBatteryDots(batteryPercent);

  } while (display.nextPage());

  Serial.println("Vertical grid rendered");
}

void renderSpecialDay(Config::SpecialDay& specialDay, int batteryPercent) {
  display.setFullWindow();
  display.firstPage();

  do {
    display.fillScreen(GxEPD_WHITE);

    // Split quote and attribution at newline
    String fullQuote = specialDay.quote;
    int newlineIndex = fullQuote.indexOf('\n');
    String quote = fullQuote;
    String attribution = "";

    if (newlineIndex >= 0) {
      quote = fullQuote.substring(0, newlineIndex);
      attribution = fullQuote.substring(newlineIndex + 1);
      attribution.trim();  // Remove any extra whitespace
    }

    // Large serif font for quote with word wrap
    display.setFont(&FreeSerif24pt7b);
    display.setTextColor(GxEPD_BLACK);

    int16_t x1, y1;
    uint16_t w, h;
    const int maxWidth = DISPLAY_WIDTH - 60;  // 30px margins each side
    const int lineHeight = 48;

    // Word-wrap the quote into lines
    String lines[10];
    int lineCount = 0;
    String remaining = quote;

    while (remaining.length() > 0 && lineCount < 10) {
      // Try the full remaining text first
      display.getTextBounds(remaining, 0, 0, &x1, &y1, &w, &h);
      if ((int)w <= maxWidth) {
        lines[lineCount++] = remaining;
        break;
      }

      // Find the last space that fits within maxWidth
      int lastSpace = -1;
      for (int i = 1; i <= (int)remaining.length(); i++) {
        String sub = remaining.substring(0, i);
        display.getTextBounds(sub, 0, 0, &x1, &y1, &w, &h);
        if ((int)w > maxWidth) break;
        if (remaining.charAt(i - 1) == ' ') lastSpace = i - 1;
      }

      if (lastSpace <= 0) {
        // No space found that fits — force break at width
        lines[lineCount++] = remaining;
        break;
      }

      lines[lineCount++] = remaining.substring(0, lastSpace);
      remaining = remaining.substring(lastSpace + 1);
    }

    // Calculate total height for vertical centering
    int totalLines = lineCount + (attribution.length() > 0 ? 1 : 0);
    int totalHeight = totalLines * lineHeight;
    int startY = (DISPLAY_HEIGHT - totalHeight) / 2 + lineHeight;  // +lineHeight for baseline

    // Draw wrapped quote lines centered
    for (int i = 0; i < lineCount; i++) {
      display.getTextBounds(lines[i], 0, 0, &x1, &y1, &w, &h);
      display.setCursor((DISPLAY_WIDTH - w) / 2, startY + (i * lineHeight));
      display.print(lines[i]);
    }

    // Draw attribution below quote (never wrapped, smaller font)
    if (attribution.length() > 0) {
      display.setFont(&FreeSerif18pt7b);
      display.getTextBounds(attribution, 0, 0, &x1, &y1, &w, &h);
      display.setCursor((DISPLAY_WIDTH - w) / 2, startY + (lineCount * lineHeight));
      display.print(attribution);
    }

    // WiFi status in top left
    drawWiFiStatus();

    // Battery dots still visible on special days
    drawBatteryDots(batteryPercent);

  } while (display.nextPage());

  Serial.println("Special day rendered");
}

void renderError(const char* message) {
  display.setFullWindow();
  display.firstPage();

  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeSerif18pt7b);
    display.setTextColor(GxEPD_BLACK);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(message, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((DISPLAY_WIDTH - w) / 2, DISPLAY_HEIGHT / 2);
    display.print(message);
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
