#!/bin/bash
# Fast upload script for reTerminal E1001
# Usage: Put device in bootloader mode (BOOT+RESET), then run: ./upload.sh

PORT="/dev/cu.usbserial-110"
BAUD="921600"

SKETCH_DIR="/Users/jz/Desktop/MementoMori/arduino/MementoMori/build/esp32.esp32.XIAO_ESP32S3"

echo "🚀 Uploading firmware to reTerminal E1001..."
echo "⏱️  This should take about 10-15 seconds"
echo ""

python3 -m esptool \
  --chip esp32s3 \
  --port "$PORT" \
  --baud "$BAUD" \
  --before no_reset \
  --after no_reset \
  write_flash \
  -z \
  --flash_mode dio \
  --flash_freq 80m \
  --flash_size 8MB \
  0x0 "$SKETCH_DIR/MementoMori.ino.bootloader.bin" \
  0x8000 "$SKETCH_DIR/MementoMori.ino.partitions.bin" \
  0xe000 "$SKETCH_DIR/boot_app0.bin" \
  0x10000 "$SKETCH_DIR/MementoMori.ino.bin"

echo ""
echo "✅ Upload complete!"
