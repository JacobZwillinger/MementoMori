#!/bin/bash
# Simplified upload using merged binary at slower baud rate
# Usage: Put device in bootloader mode (BOOT+RESET), then run: ./upload-simple.sh

PORT="/dev/cu.usbserial-110"
BAUD="115200"  # Much slower, more reliable

MERGED_BIN="/Users/jz/Desktop/MementoMori/arduino/MementoMori/build/esp32.esp32.XIAO_ESP32S3/MementoMori.ino.merged.bin"

echo "🚀 Uploading firmware to reTerminal E1001 (slow but reliable mode)..."
echo "⏱️  This will take about 30-60 seconds due to slower speed"
echo ""

python3 -m esptool \
  --chip esp32s3 \
  --port "$PORT" \
  --baud "$BAUD" \
  --before no_reset \
  --after no_reset \
  write_flash \
  0x0 "$MERGED_BIN"

echo ""
echo "✅ Upload complete! Press RESET button on device to boot."
