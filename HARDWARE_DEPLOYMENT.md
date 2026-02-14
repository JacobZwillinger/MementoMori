# Hardware Deployment Guide
## Deploying Memento Mori to reTerminal E1001

This guide walks you through deploying the Memento Mori firmware to your reTerminal E1001 e-paper display.

---

## Prerequisites

### 1. Install Arduino IDE 2.x

Download from: https://www.arduino.cc/en/software

### 2. Install ESP32 Board Support

1. Open Arduino IDE
2. Go to **File → Preferences**
3. In "Additional Board Manager URLs", add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "esp32"
6. Install "esp32 by Espressif Systems" (version 2.0.11 or later)

### 3. Install Required Libraries

Go to **Tools → Manage Libraries** and install:

- **GxEPD2** by Jean-Marc Zingg (for e-paper display)
- **Adafruit GFX Library** by Adafruit (graphics primitives)
- **ArduinoJson** by Benoit Blanchon (version 6.x)

### 4. Install SPIFFS Upload Tool

1. Download ESP32 Filesystem Uploader:
   https://github.com/me-no-dev/arduino-esp32fs-plugin/releases
2. Extract to `~/Documents/Arduino/tools/`
3. Restart Arduino IDE

---

## Configuration

### 1. Update WiFi Credentials

Edit `arduino/data/config.json`:

```json
{
  "wifi": {
    "ssid": "YOUR_WIFI_SSID",
    "password": "YOUR_WIFI_PASSWORD",
    "ntpServer": "pool.ntp.org"
  }
}
```

**Important:** The device needs WiFi only for initial time sync. It will automatically disconnect after syncing to save power.

### 2. Verify Personal Settings

Check that your birthdate and special days are correct in `config.json`:

```json
{
  "person": {
    "birthdate": "1987-08-17",
    "expectedLifespan": 80,
    "timezone": "America/New_York"
  }
}
```

---

## Upload Process

### Step 1: Connect Hardware

1. Connect reTerminal E1001 to your computer via USB-C
2. Power on the device (if needed)
3. Wait for the device to be recognized

### Step 2: Select Board and Port

1. Open `arduino/MementoMori.ino` in Arduino IDE
2. Go to **Tools → Board → esp32**
3. Select **"XIAO_ESP32S3"** (or "ESP32S3 Dev Module" if XIAO isn't available)
4. Go to **Tools → Port**
5. Select the port showing your reTerminal (usually `/dev/cu.usbmodem*` on Mac)

### Step 3: Upload Filesystem (SPIFFS)

**This uploads the config.json file to the device:**

1. Go to **Tools → ESP32 Sketch Data Upload**
2. Wait for upload to complete (30-60 seconds)
3. You should see: "SPIFFS Image Uploaded"

### Step 4: Upload Sketch

1. Click the **Upload** button (→) or press **Cmd+U** (Mac) / **Ctrl+U** (Windows)
2. Wait for compilation and upload
3. Monitor progress in the console at the bottom

### Step 5: Monitor Serial Output

1. Open **Tools → Serial Monitor** or press **Cmd+Shift+M**
2. Set baud rate to **115200**
3. You should see output like:

```
Memento Mori - Initializing...
Loading configuration...
Configuration loaded successfully
Birthdate: 1987-08-17
Special days: 4
Connecting to WiFi: YOUR_SSID
...
WiFi connected
Syncing time...
Time synchronized
Friday, August 16 2026 08:30:45
Weeks lived: 2028 of 4160
Grid rendered
Sleeping for 55515 seconds until midnight
Entering deep sleep...
```

---

## Verification Checklist

After upload, verify:

- [ ] Serial output shows "Configuration loaded successfully"
- [ ] WiFi connects (watch for "WiFi connected")
- [ ] Time syncs (shows current date/time in Eastern timezone)
- [ ] Week count is correct (~2028 weeks for birthdate Aug 17, 1987)
- [ ] E-paper display updates with the grid
- [ ] Device enters deep sleep

---

## Troubleshooting

### Upload Failed

**Error:** `Failed to connect to ESP32: No serial data received.`

**Solution:**
- Hold the **BOOT** button on the device while clicking Upload
- Release BOOT when you see "Connecting..."
- Try a different USB cable (some cables are charge-only)

### SPIFFS Upload Failed

**Error:** `SPIFFS was not declared in this scope`

**Solution:**
- Use "ESP32 Sketch Data Upload" from Tools menu
- If not visible, reinstall the SPIFFS plugin
- Make sure `arduino/data/` folder exists with `config.json` inside

### WiFi Connection Failed

**Error:** `WiFi connection failed` in serial monitor

**Solution:**
- Double-check SSID and password in `config.json`
- Ensure device is in range of WiFi
- Try 2.4GHz network (ESP32 doesn't support 5GHz)
- Re-upload SPIFFS with corrected credentials

### Time Sync Failed

**Error:** `Time sync failed`

**Solution:**
- Check WiFi connection first
- Try different NTP server: `time.nist.gov` or `time.google.com`
- Check firewall settings (NTP uses UDP port 123)

### Display Not Updating

**Symptoms:** Serial monitor works, but display stays blank

**Solution:**
- Check GxEPD2 library is installed correctly
- Verify board selection (must be ESP32-S3)
- Check display cable connections
- Try pressing the reset button on the device

### Wrong Week Count

**Symptoms:** Week count doesn't match expected value

**Solution:**
- Verify birthdate in `config.json` is correct format: `YYYY-MM-DD`
- Check that time sync succeeded (Eastern timezone)
- Manually calculate: (current age × 52) + weeks since last birthday

---

## Battery Life Optimization

The device is configured for maximum battery life:

- **Deep sleep** between daily updates (~10-50μA)
- **WiFi off** except during sync
- **NTP sync** only weekly (configurable)
- **Expected battery life:** 6-12 months on 2000mAh battery

### Monitoring Battery

To check battery status, connect via serial during wake-up and add this to the code:

```cpp
// In setup(), before deep sleep:
float voltage = analogRead(A0) * 2.0 * 3.3 / 4096.0;
Serial.print("Battery voltage: ");
Serial.println(voltage);
```

---

## Updating Configuration

To change special days or quotes without re-uploading code:

1. Edit `arduino/data/config.json`
2. Run **Tools → ESP32 Sketch Data Upload**
3. Device will use new config on next midnight update

**No need to re-upload the sketch!**

---

## Advanced: Custom Display Times

Default: Updates at midnight Eastern Time daily.

To change update time, edit in the sketch:

```cpp
// In calculateSecondsUntilUpdate():
int targetHour = 0;  // Change to desired hour (0-23)
```

---

## Next Steps

Once deployed and verified:

1. **Place the device** where you'll see it daily
2. **Let it run** - it will wake up at midnight Eastern and update
3. **Check special days** - on Aug 17 (your birthday), you'll see the Seneca quote
4. **Monitor battery** monthly for the first few months

---

## Support

If you encounter issues not covered here:

1. Check the serial monitor output for specific error messages
2. Review the GitHub issues: https://github.com/JacobZwillinger/MementoMori/issues
3. Verify all library versions are compatible (see Prerequisites)

---

## Hardware Specifications

**reTerminal E1001:**
- Display: 7.5" 800×480 monochrome e-paper
- Processor: ESP32-S3 (dual-core, 240MHz)
- RAM: 512KB SRAM
- Flash: 8MB
- Connectivity: WiFi 802.11 b/g/n, Bluetooth 5.0
- Power: Built-in battery management, 2000mAh battery
- Refresh time: ~2 seconds full refresh

---

**Your life is finite. Make each week count.**
